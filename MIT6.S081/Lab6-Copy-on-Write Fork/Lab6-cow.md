

# 一、实验目标

`fork()` 不再立即拷贝物理页，而是**父子共享同一物理页**，把该页的 PTE 改成**只读**并打上我们自定义的 **COW 标记**。当任何一方对该页发生写（或内核中的 `copyout` 对用户页写）触发页故障，再**按需分配**一个新物理页，复制旧内容，更新映射并恢复可写。

---

# 二、实验流程

## 0. 新增一个 PTE 标志位（COW 标记）

**文件**：`kernel/riscv.h`  
**位置**：和其它 PTE_* 常量放一起（`PTE_V、PTE_R、PTE_W...` 的附近）

> 说明：RISC-V 页表项的某些位（比如 bit8/bit9）留给软件使用。我们用 bit8 做 COW 标记。

```c
// 在其他 PTE_* 宏后面加一行（推荐放在 PTE_D 之后）
#define PTE_F (1L << 8)   // 自定义：COW 标记（F = forked via COW）
```

> xv6 的 `PTE_FLAGS(pte)` 默认保留低 10 位（0x3FF），会把我们自定义的 bit8 一起保留下来，正合适。

---
## 1. 为物理页建立**全局引用计数**（kalloc.c）

我们需要知道每个**物理页**被多少进程（页表映射）共享。只有引用计数降为 0 才真正释放。用**自旋锁**保护以避免多核同时修改。

**文件**：`kernel/kalloc.c`

### 1.1 定义全局 ref 并初始化锁

**放在文件顶部**（`struct run`、`kmem` 附近，全局作用域）：

```c
struct ref_stru {
  struct spinlock lock;
  int cnt[PHYSTOP / PGSIZE];  // 物理页引用计数数组
} ref;
```

**在 kinit() 中初始化**（就在对 `kmem.lock` 初始化附近加一行）：

```c
void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&ref.lock, "ref");     // <--- 新增
  freerange(end, (void*)PHYSTOP);
}
```

### 1.2 修改 freerange：给每个页预置 ref=1 再 kfree

> 原因：`kfree()` 里会先减引用计数；如果现在是 0，会减成负数。所以在 `freerange()` 里先把 ref 设成 1。

把原来的 `freerange()` 改成这样：

```c
void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    ref.cnt[(uint64)p / PGSIZE] = 1; // 先置为1，方便 kfree 内部 --cnt
    kfree(p);
  }
}
```

### 1.3 修改 kfree：仅在**引用计数变为 0**时真正释放

修改原 `kfree()` 为：

```c
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // 只有当引用计数减到 0 才真正回收
  acquire(&ref.lock);
  if(--ref.cnt[(uint64)pa / PGSIZE] == 0) {
    release(&ref.lock);

    r = (struct run*)pa;

    // 用垃圾填充帮助发现悬挂引用
    memset(pa, 1, PGSIZE);

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  } else {
    release(&ref.lock);
  }
}
```

### 1.4 修改 kalloc：分配到页时把 ref 初始化为 1

修改 `kalloc()` 为：

```c
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r) {
    kmem.freelist = r->next;
    acquire(&ref.lock);
    ref.cnt[(uint64)r / PGSIZE] = 1;  // 新分配页的引用计数 = 1
    release(&ref.lock);
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // 填充垃圾
  return (void*)r;
}
```

### 1.5 新增 2 个小工具函数：获取/增加引用计数

仍在 `kalloc.c` 末尾（或合适位置）新增：

```c
int
krefcnt(void *pa)
{
  return ref.cnt[(uint64)pa / PGSIZE];   // 读不加锁也可；保守起见可加锁
}

int
kaddrefcnt(void *pa)
{
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    return -1;
  acquire(&ref.lock);
  ++ref.cnt[(uint64)pa / PGSIZE];
  release(&ref.lock);
  return 0;
}
```

---

## 2. 在 vm.c 里实现 COW 支持函数 + 改造 copyout/uvmcopy

**文件**：`kernel/vm.c`

## 2.1 声明：在 `kernel/defs.h` 加这些原型

```c
// kalloc.c
int krefcnt(void *pa);
int kaddrefcnt(void *pa);

// vm.c（本文件内实现）
int  cowpage(pagetable_t pagetable, uint64 va);
void *cowalloc(pagetable_t pagetable, uint64 va);
```

## 2.2 工具函数：判断某 VA 是否 COW 页

加在 `vm.c`（通常在文件任意函数区）：

```c
// 是 COW 页返回 0；不是返回 -1（模仿 xv6 常用风格）
int
cowpage(pagetable_t pagetable, uint64 va)
{
  if(va >= MAXVA)
    return -1;
  pte_t *pte = walk(pagetable, va, 0);
  if(pte == 0)
    return -1;
  if((*pte & PTE_V) == 0)
    return -1;
  return (*pte & PTE_F) ? 0 : -1;
}
```

## 2.3 核心：cowalloc（写时分配/换页）

当发现写到 COW 页，按两种情况处理：

- **该物理页仅我在用（ref==1）**：直接把当前 PTE 的 `PTE_W` 打开，去掉 `PTE_F`，不用复制。
    
- **被多人共享（ref>1）**：`kalloc()` 新页，`memmove()` 拷贝，改映射到新页，旧页 `kfree()`（只会把 ref--）。
    

```c
void*
cowalloc(pagetable_t pagetable, uint64 va)
{
  if(va % PGSIZE != 0)
    return 0;

  uint64 pa = walkaddr(pagetable, va);
  if(pa == 0)
    return 0;

  pte_t *pte = walk(pagetable, va, 0);
  if(pte == 0)
    return 0;

  if(krefcnt((void*)pa) == 1){
    // 只有我在用：直接“解封”可写，去掉 COW 标记
    *pte |= PTE_W;
    *pte &= ~PTE_F;
    return (void*)pa;
  } else {
    // 多人共享：分配新页，复制，再重新映射
    char *mem = kalloc();
    if(mem == 0)
      return 0;

    memmove(mem, (char*)pa, PGSIZE);

    // 暂时清掉 PTE_V，避免 mappages 认为是 remap
    uint flags = PTE_FLAGS(*pte);
    *pte &= ~PTE_V;

    if(mappages(pagetable, va, PGSIZE, (uint64)mem, (flags | PTE_W) & ~PTE_F) != 0){
      kfree(mem);
      *pte |= PTE_V;
      return 0;
    }

    // 旧物理页的引用计数 --（不一定释放）
    kfree((char*)PGROUNDDOWN(pa));
    return mem;
  }
}
```

## 2.4 改造 uvmcopy：不再复制物理页，改为共享并打 COW 标记

**函数**：`uvmcopy(pagetable_t old, pagetable_t new, uint64 sz)`

把原先「给子进程分配新页再 memcpy」的逻辑替换为「共享同一物理页 + 清写位 + 打 COW 标记 + ref++」。

> 仅对**原来是可写的页**打 COW（只读页无需 COW）。

```c
int
uvmcopy(pagetable_t old, pagetable_t new, uint64 sz)
{
  pte_t *pte;
  uint64 pa, i;
  uint flags;

  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walk(old, i, 0)) == 0)
      panic("uvmcopy: pte should exist");
    if((*pte & PTE_V) == 0)
      panic("uvmcopy: page not present");
    pa = PTE2PA(*pte);
    flags = PTE_FLAGS(*pte);

    // 对“原本可写”的页启用 COW：清 PTE_W，置 PTE_F
    if(flags & PTE_W){
      flags = (flags | PTE_F) & ~PTE_W;
      *pte = PA2PTE(pa) | flags; // 父进程自己的 PTE 也要清写位并打标
    }

    if(mappages(new, i, PGSIZE, pa, flags) != 0){
      uvmunmap(new, 0, i / PGSIZE, 1);
      return -1;
    }

    // 引用计数 +1（父子现在共享了这个物理页）
    kaddrefcnt((void*)pa);
  }
  return 0;
}
```

## 2.5 改造 copyout：内核向用户写时也会触发 COW

**函数**：`copyout(pagetable_t pagetable, uint64 dstva, char *src, uint64 len)`

只改动循环里拿到 `pa0` 后、真正写入前的部分：如目标页是 COW，先 `cowalloc()` 换成可写的新页/独占页。

```c
int
copyout(pagetable_t pagetable, uint64 dstva, char *src, uint64 len)
{
  uint64 n, va0, pa0;

  while(len > 0){
    va0 = PGROUNDDOWN(dstva);
    pa0 = walkaddr(pagetable, va0);

    // 如目标是 COW 页，先写时复制
    if(cowpage(pagetable, va0) == 0){
      pa0 = (uint64)cowalloc(pagetable, va0);  // 可能返回新物理页
    }

    if(pa0 == 0)
      return -1;

    n = PGSIZE - (dstva - va0);
    if(n > len)
      n = len;

    memmove((void *)(pa0 + (dstva - va0)), src, n);

    len -= n;
    src += n;
    dstva = va0 + PGSIZE;
  }
  return 0;
}
```

> 为什么要改 `copyout`？  
> 因为 `copyout` 是**内核代表进程**往用户地址写（比如 `read()` 把数据拷到用户缓冲区）。它不会触发用户态页故障，所以得在内核里手动处理 COW。

---

## 3. 在 trap.c 里处理用户页写故障（usertrap）

**文件**：`kernel/trap.c`  
**函数**：`usertrap()`

思路：

- 读 `scause`。
    
- 如果是**load page fault**（13）或**store/AMO page fault**（15），那就看 `stval`（出错 VA）。
    
- 只有当该 VA 属于当前进程地址空间 && 是 COW 页，才进行 `cowalloc()`。失败就 `p->killed = 1`。
    

在 `usertrap()` 里加一段分支（位置在 `if(r_scause()==8)` 系统调用分支、`else if((which_dev=devintr())!=0)` 设备中断分支之外；常见写法如下）：

```c
void
usertrap(void)
{
  int which_dev = 0;
  struct proc *p = myproc();

  if((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  w_stvec((uint64)kernelvec);

  p->trapframe->epc = r_sepc();

  if(cause == 8){
    // system call ...
    // （原有代码保持）
  } else if((which_dev = devintr()) != 0){
    // 设备中断 ...
  } else if(r_scause() == 13 || r_scause() == 15){
    uint64 va = r_stval();  // 故障虚拟地址
    // 只有：地址在进程大小内 && 该页是 COW && COW 处理成功，才算恢复
    if(va >= p->sz ||
       cowpage(p->pagetable, va) != 0 ||
       cowalloc(p->pagetable, PGROUNDDOWN(va)) == 0){
      p->killed = 1;
    }
  } else {
    // 其它异常：保留原有打印和 kill
    // ...
  }

  if(p->killed)
    exit(-1);

  // 原有的时钟中断让出 CPU 等逻辑...
  usertrapret();
}
```

---

# 第 4 步：把声明加进 defs.h（很重要，别忘了）

**文件**：`kernel/defs.h`  
在相应分类下补充：

```c
// kalloc.c
void  kfree(void *);
void* kalloc(void);
int   krefcnt(void *pa);
int   kaddrefcnt(void *pa);

// vm.c
int   cowpage(pagetable_t pagetable, uint64 va);
void* cowalloc(pagetable_t pagetable, uint64 va);
```

---

## 5. 实验测试（cowtest + usertests + grade）

```bash
make clean && make qemu
```

看能否正常进入 shell，执行几个简单命令（如 `echo hi`、`ls` 等）确保不崩。


1. 跑本 lab 的 cowtest 自测：
    
```shell
hart 1 starting
hart 2 starting
init: starting sh
$ cowtest
simple: ok
simple: ok
three: ok
three: ok
three: ok
file: ok
ALL COW TESTS PASSED
```

2. 完整测试：
	
```bash
usertests
```

输出末尾应有：

```
ALL TESTS PASSED
```

1. 评分脚本：
    
```bash
make grade
```

![alt text](<Pasted image 20250831031838.png>)

---

# 三、调试与常见坑

- **页标志位**：必须只清 `PTE_W`，保留 `PTE_R|PTE_X|PTE_U|...`，并额外加 `PTE_F`。否则会把可执行或读权限弄丢，导致奇怪崩溃。
    
- **copyout 一定要改**：内核向用户写数据时不会走用户态页故障，所以不改 `copyout` 会在很多用户程序（比如 `read()`、`pipe`）中触发写入只读页的异常。
    
- **freerange 先设 1**：否则 `kfree()` 开始就 `--ref`，会把计数减成负数，早早 panic。
    
- **kfree() 参数必须页对齐**：注意我们在 `cowalloc()` 最后 `kfree(PGROUNDDOWN(pa))`，保证对齐。
    
- **并发安全**：对 `ref.cnt[]` 的增减操作都加锁；`krefcnt()`只读可以不加，但更稳妥可以也加。
    
- **PTE_FLAGS()**：xv6 2020 默认是 `pte & 0x3FF`，能带上 `PTE_F`。如果你仓库不是官方 2020 代码，确认一下（通常无需更改）。
    
- **usertrap 分支顺序**：`devintr()` 对页故障返回 0，所以把 COW 分支放在 `devintr()` 之后、默认 `else` 之前很自然；保持风格一致即可。
    
- **只给“原来可写”的页打 COW**：只读页没必要打标，也不会写时复制。
    
