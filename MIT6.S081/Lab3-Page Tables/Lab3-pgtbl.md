# 一、实验准备

## 1.1 常用命令与条件

- 在 xv6 源码目录下操作（例如 `~/xv6`）。
- 常用命令：
    - 编译并运行：`make qemu`
    - 运行 lab 打分（针对 pte printout）：`./grade-lab-pgtbl pte printout`
    - 运行全部用户测试：在 qemu 的 shell 中运行 `usertests` 或在宿主机运行测试脚本（视仓库提供方式）
    - 总体测试：`make grade`


## 1.2 分支管理

```bash
cd ~/6.s081-xv6-labs   # 按你的路径
git fetch
git checkout pgtbl           # 实验主分支
# 如果你想先在副本试验：
# git checkout -b pgtbl_test
make clean && make qemu         # 确认能启动，Ctrl+A X 退出
```

进行分支管理，方便实验记录或失败后重来

- 创建实验分支

```shell
git checkout -b pgtbl_test
```

---

# 二、Print a page table（easy）

**目标**：实现 `vmprint()`，递归打印传入的页表内容，并在 `exec()` 中打印 PID == 1 的进程页表（用于调试/打分）。

### 1. 在 `kernel/defs.h` 中声明函数

在 `defs.h`现有 vm.c 函数原型附近，新增：

```c
// vm.c
void            kvminit(void);
...
void            vmprint(pagetable_t); // 新增
```

### 2. 在 `kernel/vm.c` 的文件末尾新增实现

在 `vm.c` 文件底部（其它 vm 相关函数实现之后），加入下面两个函数：`vmprint_recursive()` 与 `vmprint()`。

```c
// 在 vm.c 文件末尾新增

// 递归打印页表（内部使用）
void
vmprint_recursive(pagetable_t pagetable, int level) {
    // xv6-book 风格：L2（顶级） -> L1 -> L0
    static int levels[] = {0, 2, 1, 0};

    // 每个 PTE 8 字节，512 个 PTE
    for (int i = 0; i < PGSIZE / 8; i++) {
        pte_t pte = pagetable[i];
        if (pte & PTE_V) {
            // 缩进显示层级
            for (int j = 0; j < level; j++)
                printf(" ..");

            uint64 pa = PTE2PA(pte);
            printf("%d: pte %p pa %p\n", i, pte, pa);

            // 用来说明这是指向下一级页表还是实际物理页
            printf("level= %d, is it pgtbl? %s\n",
                   levels[level],
                   ((pte & (PTE_R | PTE_W | PTE_X)) == 0) ? "true" : "false");

            // 如果这个 PTE 表示下一级页表（即无 R/W/X），递归打印
            if ((pte & (PTE_R | PTE_W | PTE_X)) == 0) {
                vmprint_recursive((pagetable_t) pa, level + 1);
            }
        }
    }
}

void
vmprint(pagetable_t pagetable) {
    printf("page table %p\n", pagetable);
    vmprint_recursive(pagetable, 1);
}
```

**说明**：
- `PTE2PA()` 宏将页表项转换为物理地址，常见定义为 `#define PTE2PA(pte) (((pte) >> 10) << 12)`（把物理页号移出来并左移到物理地址）。
- 递归从顶级页表（L2）开始，逐层打印有效的 PTE；当 PTE 表示下一级页表时（没有 R/W/X 权限位），递归继续。 
  

### 3. 在 `kernel/exec.c` 的 `exec()` 中调用 `vmprint()`

找到 `exec()` 函数的末尾（在 `return argc;` 之前），插入：

```c
int
exec(char *path, char **argv)
{
  char *s, *last;
  ...
  // 打印第一个进程（pid == 1）的页表，便于调试/打分
  if(p->pid == 1)
    vmprint(p->pagetable);
  
  return argc;
}
```


### 4. 测试

- `make qemu`，启动后查看输出应该包含 `page table 0x...` 及递归打印的内容。
- 运行打分：`./grade-lab-pgtbl pte printout`。出现 `pte printout: OK`通过。


![alt text](<lab3-assert/Pasted image 20250818173853.png>)

---



# 三、A kernel page table per process（hard）

**目标**：每个进程在内核态执行时使用自己的“内核页表副本”（kpgtbl），原理是为每个进程创建一个内核页表（其内容与原全局 `kernel_pagetable` 相同），并在上下文切换时切换 SATP 到进程的 kpgtbl，切回调度器时恢复全局内核页表。

## 3.1 总体思路

1. **初始化改造**：将全局内核映射初始化封装到 `vminit(pagetable_t)`，并修改 `kvminit()` 以创建 `kernel_pagetable` 并调用 `vminit()`。
    
2. **进程创建/回收**：在 `allocproc()` 时创建 `kpgtbl`（用户内核页表，kernel page table for user process）并为该进程分配内核栈页并通过 `vmmap(kpgtbl, ...)` 映射；在 `freeproc()` 时释放这些资源。
    
3. **调度/切换**：在 `scheduler()` 中，切换到进程前 `w_satp(MAKE_SATP(p->kpgtbl)); sfence_vma();`，运行后再切回 `kernel_pagetable`。
    

## 3.2 实验流程
### A) `kernel/defs.h` 新增/修改原型

在 vm/kvm 相关声明附近加入（或修改）：

```c
void            kvminit(void);
void            kvminithart(void);
uint64          kvmpa(pagetable_t pagetable, uint64 va); // 加入pagetable 参数
...
void            vmprint(pagetable_t);
void            vminit(pagetable_t); // 新增：把内核映射逻辑搬到此函数，可用于任意页表
int             vmmap(pagetable_t, uint64, uint64, uint64, int); // 新签名，增加 pagetable 参数
pagetable_t     createukpgtbl(void);
void            freeukpgtbl(pagetable_t);
```

### B) `kernel/vm.c` 的关键修改

1. **拆分初始化**：把原来 `kvminit()` 的映射部分提取到 `vminit(pagetable_t)`。示例：

```c
/*
 * create a direct-map page table for the kernel. 
 */
void
kvminit() {
    kernel_pagetable = (pagetable_t) kalloc();
    // 一堆映射工作改到了 vminit() 中
    vminit(kernel_pagetable);
}

// 新增函数
void
vminit(pagetable_t pagetable) {
    memset(pagetable, 0, PGSIZE);

    // uart registers
    vmmap(pagetable, UART0, UART0, PGSIZE, PTE_R | PTE_W);

    // virtio mmio disk interface
    vmmap(pagetable, VIRTIO0, VIRTIO0, PGSIZE, PTE_R | PTE_W);

    // CLINT -> 仅为全局 kernel_pagetable 映射，详见后文
    vmmap(pagetable, CLINT, CLINT, 0x10000, PTE_R | PTE_W);

    // PLIC
    vmmap(pagetable, PLIC, PLIC, 0x400000, PTE_R | PTE_W);

    // kernel text (R/X)
    vmmap(pagetable, KERNBASE, KERNBASE, (uint64) etext - KERNBASE, PTE_R | PTE_X);

    // kernel data and physical RAM
    vmmap(pagetable, (uint64) etext, (uint64) etext, PHYSTOP - (uint64) etext, PTE_R | PTE_W);

    // trampoline
    vmmap(pagetable, TRAMPOLINE, (uint64) trampoline, PGSIZE, PTE_R | PTE_X);
}
```

> **注意**：为了避免后面 u2kvmcopy 的“重复映射 remap 错误”，在 `vminit` 中要加一个判断：只有当 `pagetable == kernel_pagetable` 时才映射 CLINT（或根据你的仓库需要改成 `if (pagetable == kernel_pagetable)` 包裹 `vmmap(pagetable, CLINT, ...)`）。这样用户进程的 kpgtbl 不会预先映射 CLINT，从而避免 `u2kvmcopy()` 对 CLINT 再次映射时发生冲突。


2. **实现 `vmmap()`**（将 `mappages()` 封装并传入 pagetable）：

```c
int
vmmap(pagetable_t pagetable, uint64 va, uint64 pa, uint64 sz, int perm) {
    if (mappages(pagetable, va, sz, pa, perm) != 0) {
        if (pagetable == kernel_pagetable) {
            panic("vmmap");
        }
        return -1;
    }
    return 0;
}
```


3. **修改 `kvmpa()` 签名**，以接受 `pagetable` 参数（查找给定 pagetable 中 va 对应的物理地址）：
```c
// 新增参数
uint64
kvmpa(pagetable_t pagetable, uint64 va) {
    uint64 off = va % PGSIZE;
    pte_t *pte;
    uint64 pa;

    pte = walk(pagetable, va, 0); // 改为使用入参
    if (pte == 0)
        panic("kvmpa");
    ...
}
```

**`kernel/virtio_disk.c`改用进程 kpgtbl：**

在使用 `kvmpa()` 的地方改为传入 `myproc()->kpgtbl`，例如：

```c
...
#include "virtio.h"
#include "proc.h" // 导入

void
virtio_disk_rw(struct buf *b, int write)
{
  ...
  buf0.sector = sector;

  // buf0 is on a kernel stack, which is not direct mapped,
  // thus the call to kvmpa().
  disk.desc[idx[0]].addr = (uint64) kvmpa(myproc()->kpgtbl, (uint64) &buf0); // 使用用户内核页查找物理地址 myproc()->kpgtbl
}
```

记得在文件顶部包含 `proc.h`。



4. **创建 / 释放 用户内核页表（kpgtbl）**：
```c
// 创建一个用户进程的内核页表并完成初始化工作
pagetable_t
createukpgtbl(void) {
    pagetable_t ukpgtbl = (pagetable_t) kalloc();
    if (!ukpgtbl) return 0;
    vminit(ukpgtbl);
    return ukpgtbl;
}

// 释放三级页表占用的内存，共 3*4kb
void
freeukpgtbl(pagetable_t pagetable) {
    // 释放三级页表占用的页（只释放页表本身占用的页面，不释放页表项映射的物理内存）
    for (int i = 0; i < 512; i++) {
        pte_t l1pte = pagetable[i];
        if ((l1pte & PTE_V) && (l1pte & (PTE_R|PTE_W|PTE_X)) == 0) {
            uint64 l1ptepa = PTE2PA(l1pte);
            // 释放 l1 页表内指向的 l0 页表
            for (int j = 0; j < 512; j++) {
                pte_t l0pte = ((pagetable_t)l1ptepa)[j];
                if ((l0pte & PTE_V) && (l0pte & (PTE_R|PTE_W|PTE_X)) == 0) {
                    kfree((void*)PTE2PA(l0pte)); // 释放 l0 页表页
                }
            }
            kfree((void*)l1ptepa); // 释放 l1 页表页
        }
    }
    kfree(pagetable); // 释放 l2 页表页
}
```

### C) `kernel/proc.h`：在 `struct proc` 中新增字段

在 `proc.h` 的 `struct proc` 中加入 `kpgtbl`字段

```c
struct proc {
  struct spinlock lock;
  ...
  char name[16];
  pagetable_t kpgtbl;   // 新增：进程对应的内核页表副本
  ...
};
```

### D) `kernel/proc.c`：删除 procinit 里为每个进程预分配内核栈（将分配移到 allocproc）

**删除/注释掉** `procinit()` 中为每个进程预先 `kalloc()` 内核栈并映射到全局 kernel_pagetable 的那段代码。即把这段从 `procinit()` 中删掉/注释：

```c
// 注释掉（或删除）原来在 procinit() 中为每个 proc 预分配 kstack 的代码：
// char *pa = kalloc();
// if(pa == 0) panic("kalloc");
// uint64 va = KSTACK((int) (p - proc));
// kvmmap(va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
// p->kstack = va;
```

### E) `kernel/proc.c`：修改 `allocproc()`，在创建进程时创建 kpgtbl 并分配内核栈

在 `allocproc()` 的 `p->pagetable = proc_pagetable(p);` 之后（即为用户页表准备完成后），加入：

```c
  ...
  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if(p->pagetable == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // 创建进程的内核页表（kpgtbl）
  if ((p->kpgtbl = createukpgtbl()) == 0) {
      freeproc(p);
      release(&p->lock);
      return 0;
  }

  // 为内核栈分配一页物理内存并映射到进程的 kpgtbl
  char *pa = kalloc();
  if (pa == 0) {
      freeproc(p);
      release(&p->lock);
      return 0;
  }
  uint64 va = KSTACK((int)0); // 你需要确保分配一个唯一的 KSTACK 虚拟地址，这里按原有实现方式计算
  if (vmmap(p->kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W) != 0) {
      kfree((void*)pa);
      freeproc(p);
      release(&p->lock);
      return 0;
  }
  p->kstack = va;
  ...
```

> 注意：`KSTACK((int)0)` 的 `0` 位置要替换成能为当前进程生成唯一 KSTACK 索引的表达式（原来 procinit/allocproc 里用 `(int)(p - proc)`）。保持与原始实现一致。


### F) `kernel/proc.c`：修改 `freeproc()`，释放 kpgtbl 与内核栈

在 `freeproc()` 中加入释放 kpgtbl & 内核栈的逻辑（在释放用户页表后）：

```c
static void
freeproc(struct proc *p)
{
  if(p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);
  p->pagetable = 0;
  
  void *kstack_pa = (void *)kvmpa(p->kpgtbl, p->kstack); // 找到内核栈的物理地址，释放内存
  kfree(kstack_pa);
  p->kstack = 0;

  // 调用 freeukpgtbl() 释放掉进程的内核页表
  if(p->kpgtbl)
      freeukpgtbl(p->kpgtbl);
  p->kpgtbl = 0;
}
```



### G) `kernel/proc.c`：在 `scheduler()` 中切换内核页表（SATP）

在 `scheduler()` 中选择到某 `p` 运行之前，插入 SATP 切换执行 kpgtbl 的代码，并在返回后恢复全局 `kernel_pagetable`。示例如下：

```c
void
scheduler(void)
{
   ...
   if(p->state == RUNNABLE) {
      // Switch to chosen process.  It is the process's job
      // to release its lock and then reacquire it
      // before jumping back to us.
      p->state = RUNNING;
      c->proc = p;

      // 切换到进程的内核页表
      w_satp(MAKE_SATP(p->kpgtbl));
      sfence_vma();

      swtch(&c->context, &p->context);

      // 切换回全局内核页表
      kvminithart();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;

      found = 1;
    }
}
```

## 3.3 测试

- 完成后 `make qemu` -> 在 qemu shell 中运行 `usertests`，期待测试全通过（测试时间可能有点长）。


![alt text](<lab3-assert/Pasted image 20250818212439.png>)

---



# 四、Simplify copyin/copyinstr（hard）

**目标**：实现 `copyin_new()` 和 `copyinstr_new()`（已在 `vmcopyin.c` 中给出），并让内核通过映射到每个进程的用户内核页表（kpgtbl）来直接访问用户地址。主要做法是：在创建/修改用户页表时把用户页表中用到的映射同步复制到 `kpgtbl`（函数 `u2kvmcopy()`），使得当内核执行 `copyin_new()` 时可以通过当前 SATP（即进程 kpgtbl）访问用户内存。

### A) `kernel/defs.h`：新增声明

新增，可以在 //vm.c 附近
```c
// vmcopyin.c
int             copyin_new(pagetable_t, char *, uint64, uint64);
int             copyinstr_new(pagetable_t, char *, uint64, uint64);

// vm.c
...
int             u2kvmcopy(pagetable_t upgtbl, pagetable_t kpgtbl, uint64 begin, uint64 end);
```

### B) `kernel/vm.c`：令 `copyin()` / `copyinstr()` 调用新的实现

在 `vm.c` 中把原有 `copyin()` 和 `copyinstr()` **删掉或者注释掉**，替换为对 `copyin_new()` / `copyinstr_new()` 的直接调用：

```c
int
copyin(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len) {
    return copyin_new(pagetable, dst, srcva, len);
}

int
copyinstr(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max) {
    return copyinstr_new(pagetable, dst, srcva, max);
}
```

> `copyin_new()` / `copyinstr_new()` 的实现通常放 `kernel/vmcopyin.c`。确保 `vmcopyin.c` 已加入 Makefile 编译。


### C) `kernel/vm.c`：实现 `u2kvmcopy()`

把用户页表 `upgtbl` 中 `[begin, end)` 区间的映射一页页拷贝到 `kpgtbl`，去掉 `PTE_U` 标志（kernel 中不需要用户可访问标志）。示例实现：

```c
int
u2kvmcopy(pagetable_t upgtbl, pagetable_t kpgtbl, uint64 begin, uint64 end)
{
    pte_t *pte;
    uint64 pa, i;
    uint flags;

    for (i = begin; i < end; i += PGSIZE) {
        if ((pte = walk(upgtbl, i, 0)) == 0)
            panic("u2kvmcopy: pte should exist");
        if ((*pte & PTE_V) == 0)
            panic("u2kvmcopy: page not present");
        pa = PTE2PA(*pte);
        // 去掉 PTE_U 标志
        flags = PTE_FLAGS(*pte) & (~PTE_U);
        if (mappages(kpgtbl, i, PGSIZE, pa, flags) != 0) {
            uvmunmap(kpgtbl, 0, i / PGSIZE, 0);
            return -1;
        }
    }
    return 0;
}
```

### D) 在用户页表变更的关键点同步调用 `u2kvmcopy()` / `uvmunmap()`：

需要修改的函数包括 `fork()`, `exec()`, `growproc()`, `userinit()` 等，确保 kpgtbl 与用户页表保持同步。

#### 1. `proc.c` -> `fork()`：

在成功 `uvmcopy()`（父到子） 后，加入：

```c
  np->sz = p->sz;

  // 将用户页表映射内容复制到子进程的用户内核页表
  if (u2kvmcopy(np->pagetable, np->kpgtbl, 0, np->sz) < 0) {
      freeproc(np);
      release(&np->lock);
      return -1;
  }
  
  np->parent = p; 
  ...
```

#### 2. `exec.c` -> `exec()`：

在替换了用户页表（在 `p->pagetable = pagetable;` 之前），先取消旧的 kpgtbl 映射并把新用户页表内容拷贝到 kpgtbl：

```c
int
exec(char *path, char **argv)
{
  ...
  // Save program name for debugging.
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  safestrcpy(p->name, last, sizeof(p->name));

  // 先取消旧的映射内容
  uvmunmap(p->kpgtbl, 0, PGROUNDUP(oldsz) / PGSIZE, 0);
  // 将新的用户空间的页表内容拷贝到内核页表中
  if (u2kvmcopy(pagetable, p->kpgtbl, 0, sz) < 0) {
      goto bad;
  }

  // Commit to the user image.
  oldpagetable = p->pagetable;
  p->pagetable = pagetable;
  ...
}
```

#### 3. `proc.c` -> `growproc()`：

在增加内存时（`n > 0` 成功后）调用 `u2kvmcopy()` 复制新分配的页；在减少时调用 `uvmunmap()` 解除 kpgtbl 中对应的映射：

```c
int
growproc(int n)
{
  uint sz;
  struct proc *p = myproc();

  sz = p->sz;

  // 检查是否会超过 PLIC 的地址
  if(n > 0 && sz + n >= PLIC)
      return -1;

  uint oldsz = sz;

  if(n > 0){
    if((sz = uvmalloc(p->pagetable, sz, sz + n)) == 0) {
      return -1;
    }
    // n>0，申请增加内存，我们这里在等扩容成功以后，把新增加的内存映射到用户内核页表
    u2kvmcopy(p->pagetable, p->kpgtbl, PGROUNDUP(oldsz), sz);
  } else if(n < 0){
    sz = uvmdealloc(p->pagetable, sz, sz + n);
    // 同上，释放内存以后同步映射
    uvmunmap(p->kpgtbl, PGROUNDUP(sz), (PGROUNDUP(oldsz) - PGROUNDUP(sz)) / PGSIZE, 0);
  }
  p->sz = sz;
  return 0;
}
```

#### 4. `proc.c` -> `userinit()`：

用户态第一个进程要特殊处理：在 `uvminit(p->pagetable, ...)` 后调用 `u2kvmcopy()`：

```c
// Set up first user process.
void
userinit(void)
{
  ...
  // allocate one user page and copy init's instructions
  // and data into it.
  uvminit(p->pagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE;

  // 首个用户空间进程，将用户空间页表映射到内核页表
  if (u2kvmcopy(p->pagetable, p->kpgtbl, 0, PGSIZE) < 0)
    panic("userinit: u2kvmcopy");

  // prepare for the very first "return" from kernel to user.
  p->trapframe->epc = 0;      // user program counter
  p->trapframe->sp = PGSIZE;  // user stack pointer
  ...
}
```

---

### E) 处理 CLINT 重映射问题

`vminit()` 如果在每个 `kpgtbl` 中也映射 CLINT，会导致 `u2kvmcopy()` 在遇到 CLINT 地址时重映射失败。解决方法是在 `vminit()` 中仅对 `kernel_pagetable` 做 CLINT 映射：

```c
void vminit(pagetable_t pagetable) {
    memset(pagetable, 0, PGSIZE);
    ...
    
    vmmap(pagetable, UART0, UART0, PGSIZE, PTE_R | PTE_W);
    
    // virtio mmio disk interface
    vmmap(pagetable, VIRTIO0, VIRTIO0, PGSIZE, PTE_R | PTE_W);

    // 仅 kernel_pagetable 才映射 CLINT（防止 u2kvmcopy 重复映射）
    if (pagetable == kernel_pagetable) {
	    // CLINT
        vmmap(pagetable, CLINT, CLINT, 0x10000, PTE_R | PTE_W);
    }
	
	// PLIC
    vmmap(pagetable, PLIC, PLIC, 0x400000, PTE_R | PTE_W);
    ...
}
```

### 测试

```shell
make qemu
# 进入后运行
usertests
# 全部 PASS 则 OK
```

---

# 五、测试（整体）合并

- 新增文件 `time.txt`写入实验用时和文件 `answers-pgtbl.txt` 回答实验中的两个提问
- 运行 `make grade`

![alt text](<lab3-assert/Pasted image 20250826180136.png>)

补充说明：

在实验测试时遇到类似于：

```
== Test   usertests: all tests == 
  usertests: all tests: FAIL 
    ...
         test bigfile: OK
         test dirfile: OK
         test iref: OK
         test forktest: OK
         test bigdir: qemu-system-riscv64: terminating on signal 15 from pid 75364 (make)
    MISSING '^ALL TESTS PASSED$'
```

如果单独测试 `usertests` 能够全部通过，但整体测试仍然失败，通常是由于测试超时导致的。

解决方案：

- **修改测试脚本中的超时时长**，可以增加 `usertests` 测试的超时限制。因为某些测试可能需要更长时间才能完成，特别是涉及到大文件或复杂操作时。
    
- **修改 `grade-lab-pgtbl` 文件中的超时时间**，具体操作是调整 `@test(0, "usertests")` 中的 `timeout` 参数。如下所示：
    

```python
@test(0, "usertests")
def test_usertests():
    r.run_qemu(shell_script([
        'usertests'
    ]), timeout=400)  # 修改这里的超时时长，适当增加为400秒或更多
```

通过增大 `timeout`，可以避免由于测试运行时间过长而中途被强制终止，从而导致测试结果为“失败”。

- **如果还是有问题，可以检查系统资源**，比如 CPU 使用率或内存占用，看看是否有其他资源瓶颈影响了测试运行。


**实验成功合并到主分支**

```bash
git checkout pgtbl       # 回到主分支
git merge pgtbl_test    # 合并实验分支的改动
```

- 如果有冲突，Git 会提示手动解决
- 合并后实验改动就永久保存在分支 `pgtbl`
    
如果确认 `pgtbl_test` 分支的改动已经成功合并到主分支 `pgtbl` 且不再需要该分支时，可以安全地删除它。

删除本地分支的命令：

```bash
git checkout main 
git branch -d pgtbl_test
```

如果需要同时删除远程仓库（比如 GitHub 上）的 `pgtbl_test` 分支，可以执行：
```bash
git push github --delete pgtbl_test
```

---

# 六、参考

- https://blog.csdn.net/yihaiyangbaobao/article/details/146478374
- [Lab3: Pgtbl · 6.S081 All-In-One](https://xv6.dgs.zone/labs/answers/lab3.html)