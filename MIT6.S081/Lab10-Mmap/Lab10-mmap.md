
### 实验概述

`mmap` 和 `munmap` 系统调用允许程序将文件或其他对象映射到其地址空间，从而可以像访问内存一样访问文件内容。本实验需要你在 xv6 中实现这两个系统调用，核心是 **懒加载（Lazy Allocation）**：`mmap` 调用时不立即分配物理内存，而是在进程首次访问该内存区域触发**缺页异常**时，再分配物理页、读取文件内容并建立映射。

实现围绕一个关键数据结构 **VMA (Virtual Memory Area)** 展开，它用于记录进程的每个内存映射区域的信息。

实验准备：

```bash
git fetch
git checkout mmap
make clean
```


---


### 第一步：添加系统调用骨架

首先，我们需要为 `mmap` 和 `munmap` 创建系统调用的“骨架”，使得用户程序 `mmaptest` 能够编译（虽然调用会失败）。

**1. 在 `kernel/syscall.h` 中添加系统调用号：**
```c
// kernel/syscall.h
...
#define SYS_close  21
#define SYS_mmap   22 // 新增
#define SYS_munmap 23 // 新增
```

**2. 在 `kernel/syscall.c` 中声明外部函数并更新系统调用表：**
```c
// kernel/syscall.c
...
extern uint64 sys_uptime(void);
extern uint64 sys_mmap(void);   // 新增
extern uint64 sys_munmap(void); // 新增

...
[SYS_mmap]    sys_mmap,   // 新增
[SYS_munmap]  sys_munmap, // 新增
```

**3. 在 `kernel/sysfile.c` 中创建空的系统调用函数：**
```c
// kernel/sysfile.c
...
uint64
sys_mmap(void)
{
  return -1; // 先返回错误，后续实现
}

uint64
sys_munmap(void)
{
  return -1; // 先返回错误，后续实现
}
```

**4. 在 `user/user.h` 中添加用户态函数声明：**
```c
// user/user.h
...
void *mmap(void *addr, int length, int prot, int flags, int fd, int offset);
int munmap(void *addr, int length);
```

**5. 在 `user/usys.pl` 中添加入口，以便生成用户态跳板代码：**
```perl
// user/usys.pl
...
entry("uptime");
entry("mmap"); 
entry("munmap"); 
```

**6. `Makefile`：把测试程序打进镜像**

在 `UPROGS= ...` 列表中，**追加**一项：

```
$U/_mmaptest\
```


**测试步骤 1:**
1.  在终端执行 `make qemu`。
2.  编译成功后，在 xv6 shell 中运行 `mmaptest`。
3.  应该会看到程序运行失败（因为我们的系统调用返回 -1），但至少它能被编译和调用。这证明系统调用的骨架已正确添加。

```shell
xv6 kernel is booting

hart 1 starting
hart 2 starting
init: starting sh
$ mmaptest
mmap_test starting
test mmap f
mmaptest: mmap_test failed: mmap (1), pid=3
$ 
```

---

### 第二步：定义 VMA 结构

我们需要一个结构来跟踪每个进程通过 `mmap` 创建的内存映射区域。

**在 `kernel/proc.h` 中修改进程结构：**

```c
// kernel/proc.h

// 每个进程最多支持的 VMA 数量
#define NVMA 16

// VMA (Virtual Memory Area) 结构
struct vma {
  int used;           // 该 VMA 是否被使用 (1 或 0)
  uint64 addr;        // 映射的起始虚拟地址
  uint64 length;      // 映射区域的长度（字节）
  int prot;           // 权限保护位 (PROT_READ, PROT_WRITE, PROT_EXEC)
  int flags;          // 映射标志 (MAP_SHARED 或 MAP_PRIVATE)
  struct file *file;  // 被映射的文件指针
  uint64 offset;      // 文件内的偏移量（字节）
};

// Per-process state
struct proc {
  ...
  struct inode *cwd;  // Current directory
  char name[16];      // Process name (debugging)
  struct vma vmas[NVMA]; // 新增：进程的 VMA 数组
};
```

*   `used`: 这是一个简单的标记，帮助我们区分数组中的 VMA 条目是空闲的还是正在被使用。这比检查 `addr` 是否为 0 更清晰。
*   `addr`, `length`: 定义了进程地址空间中的一段连续区域。
*   `prot`: 决定了进程如何访问这块内存（读、写、执行）。
*   `flags`: 决定了修改是否写回文件（`MAP_SHARED`) 或仅私有效（`MAP_PRIVATE`)。
*   `file` 和 `offset`: 指明了映射对应文件的哪一部分。

---

### 第三步：实现 `mmap` 系统调用

现在我们来填充 `sys_mmap` 函数。它的主要工作是在进程的 VMA 表中创建一个新条目，记录映射信息，而不是立即分配物理内存。

**在 `kernel/sysfile.c` 中实现 `sys_mmap`：**

```c
// kernel/sysfile.c
#include "fcntl.h" // 确保包含了这个头文件以使用 PROT_READ 等常量

uint64
sys_mmap(void)
{
  // 1. 从陷阱帧中获取系统调用参数
  uint64 addr;
  int length, prot, flags, fd, offset;
  struct file *f;

  // 使用 arg* 函数读取参数
  if(argaddr(0, &addr) < 0 || argint(1, &length) < 0 ||
     argint(2, &prot) < 0 || argint(3, &flags) < 0 ||
     argfd(4, &fd, &f) < 0 || argint(5, &offset) < 0)
    return -1;

  // 2. 参数合法性检查
  if (length <= 0) return -1;
  // 检查 prot 是否包含有效组合
  if ((prot & (PROT_READ | PROT_WRITE | PROT_EXEC)) == 0) return -1;
  // 如果要求 PROT_WRITE 且是 MAP_SHARED，但文件本身不可写，则错误
  if ((prot & PROT_WRITE) && (flags & MAP_SHARED) && !f->writable) return -1;
  // 如果要求 PROT_READ，但文件不可读，则错误
  if ((prot & PROT_READ) && !f->readable) return -1;
  // 检查文件偏移量是否页面对齐（简化实现）
  if (offset % PGSIZE != 0) return -1;

  struct proc *p = myproc();
  // 将长度向上取整到页面大小
  length = PGROUNDUP(length);

  // 3. 检查进程地址空间是否会超出限制
  // 我们选择从当前进程大小 p->sz 开始映射（addr=0 的常见情况）
  uint64 proposed_addr = (addr == 0) ? p->sz : addr;
  if (proposed_addr + length > MAXVA) {
    return -1;
  }

  // 4. 在进程的 vmas 数组中找到一个空闲的槽位
  struct vma *vv = 0;
  for (int i = 0; i < NVMA; i++) {
    if (p->vmas[i].used == 0) {
      vv = &p->vmas[i];
      break;
    }
  }
  if (vv == 0) {
    return -1; // 没有空闲的 VMA 槽位
  }

  // 5. 填充找到的 VMA 结构
  vv->used = 1;
  vv->addr = proposed_addr; // 映射的起始地址
  vv->length = length;      // 映射的长度
  vv->prot = prot;          // 权限
  vv->flags = flags;        // 标志
  vv->file = f;             // 文件指针
  vv->offset = offset;      // 文件偏移

  // 6. 增加文件的引用计数，防止文件在映射时被关闭
  filedup(f);

  // 7. 如果用户没有指定地址（addr=0），我们更新进程的大小 p->sz
  //    这样后续的 mmap 或 sbrk 就会从新的地址开始。
  if (addr == 0) {
    p->sz = proposed_addr + length;
  }

  // 8. 返回映射区域的起始地址
  return proposed_addr;
}
```

*   **参数获取与检查**：这是系统调用的标准操作，确保用户传递的参数是有效的、安全的。
*   **地址选择**：当 `addr` 参数为 0 时，内核负责选择一个合适的地址。通常选择当前进程的堆顶（`p->sz`），然后更新 `p->sz`，这类似于 `sbrk` 的行为。
*   **懒加载**：最关键的一点是，这个函数**没有分配任何物理内存**，也没有建立任何页表映射。它只是记录了这个映射的“意向”（元数据）。真正的内存分配会在**缺页异常**中处理。
*   **文件引用**：`filedup(f)` 增加了文件引用计数，这意味着即使用户之后关闭了文件描述符 `fd`，底层的 `struct file` 也不会被销毁，因为 VMA 还在引用它。

**测试步骤 2:**
1.  重新编译并运行 `make qemu`。
2.  运行 `mmaptest`。
3.  此时，第一个 `mmap` 调用应该成功并返回一个地址（不再是失败），但接下来当测试程序尝试**读取**映射的内存时，会触发缺页异常，而我们的异常处理程序还未实现，导致进程被杀死。这是预期的进展！

```shell
xv6 kernel is booting

hart 1 starting
hart 2 starting
init: starting sh
$ mmaptest
mmap_test starting
test mmap f
usertrap(): unexpected scause 0x000000000000000d pid=3
            sepc=0x0000000000000076 stval=0x0000000000004000
panic: uvmunmap: not mapped
```

---

### 第四步：处理缺页异常（核心）

这是本实验最核心的部分。当进程访问一个已 `mmap` 但尚未分配物理页的地址时，会触发缺页异常（`scause=13或15`）。我们需要在异常处理程序中识别出这是对 mmap 区域的访问，然后为其分配物理页、加载文件内容、建立页表映射。

**在 `kernel/trap.c` 的 `usertrap()` 函数中添加处理逻辑：**

```c
// kernel/trap.c
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "defs.h"
#include "fs.h"
#include "file.h"
#include "fcntl.h" // 需要包含这些头文件以使用文件相关函数

void
usertrap(void)
{
  ...
  } else if((which_dev = devintr()) != 0){
    // ok
  } else {
    // 处理其他异常，主要是缺页异常
    uint64 va = r_stval(); // 获取引发缺页的虚拟地址

    // 关键：检查缺页地址是否可能属于一个 mmap 区域
    if (va >= p->sz || va < p->trapframe->sp) {
      // 地址超出进程大小或低于栈顶，是非法访问
      p->killed = 1;
      goto end;
    }

    // 将对齐到页面起始地址
    va = PGROUNDDOWN(va);

    // 遍历进程的所有 VMA，检查 va 是否落在某个 VMA 区间内
    struct vma *vma = 0;
    for (int i = 0; i < NVMA; i++) {
      if (p->vmas[i].used &&
          va >= p->vmas[i].addr &&
          va < p->vmas[i].addr + p->vmas[i].length) {
        vma = &p->vmas[i];
        break;
      }
    }

    if (vma == 0) {
      // 该地址不属于任何 mmap 区域，是非法访问
      p->killed = 1;
      goto end;
    }

    // 现在我们知道这是一个合法的 mmap 缺页
    // 1. 分配一个物理页
    char *mem = kalloc();
    if(mem == 0){
      p->killed = 1; // 内存分配失败，杀死进程
      goto end;
    }
    memset(mem, 0, PGSIZE); // 清零新页面（可选，但更安全）

    // 2. 计算文件中需要读取的偏移量
    //    va 在 VMA 中的偏移 = va - vma->addr
    //    文件中的偏移 = vma->offset + (va - vma->addr)
    uint64 file_offset = vma->offset + (va - vma->addr);
    
    // 3. 将文件内容读取到新分配的物理页
    ilock(vma->file->ip);
    int read_bytes = readi(vma->file->ip, 0, (uint64)mem, file_offset, PGSIZE);
    iunlock(vma->file->ip);

    if(read_bytes < 0){
      // 文件读取失败，清理并杀死进程
      kfree(mem);
      p->killed = 1;
      goto end;
    }
    // 如果 read_bytes < PGSIZE，页面剩余部分已经是零（因为memset）

    // 4. 根据 VMA 的权限设置 PTE 的标志位
    int pte_flags = PTE_U; // 用户模式可访问是必须的
    if(vma->prot & PROT_READ)  pte_flags |= PTE_R;
    if(vma->prot & PROT_WRITE) pte_flags |= PTE_W;
    if(vma->prot & PROT_EXEC)  pte_flags |= PTE_X;
    // 注意：即使 VMA 可写，文件也可能以只读打开。
    // 但对于 MAP_PRIVATE，我们仍然需要在 PTE 中保留可写权限以供进程修改私有副本。
    // 我们的参数检查已经确保了 MAP_SHARED 且 PROT_WRITE 时文件是可写的。

    // 5. 将物理页映射到用户的页表中
    if(mappages(p->pagetable, va, PGSIZE, (uint64)mem, pte_flags) != 0){
      // 映射失败（例如页表分配失败），清理并杀死进程
      kfree(mem);
      p->killed = 1;
    }

  end:
    ;
  }
  ...
}
```

*   **地址检查**：首先确认触发缺页的地址 `va` 在用户栈和进程大小 `p->sz` 之间，这是一个合法的用户空间地址。
*   **查找 VMA**：遍历进程的 VMA 数组，检查 `va` 是否落在某个 VMA 的地址范围内。如果找到，说明这次缺页是合法的 `mmap` 访问。
*   **分配与读取**：
    *   `kalloc()` 分配一页物理内存。
    *   `readi()` 从文件的正确偏移量（`vma->offset + (va - vma->addr)`）读取最多 `PGSIZE` 字节的内容到物理页中。`ilock`/`iunlock` 用于保护文件inode的操作。
*   **设置权限并映射**：根据 VMA 中记录的 `prot` 权限设置页表项（PTE）的标志位，然后调用 `mappages` 建立虚拟地址 `va` 到物理地址 `mem` 的映射。

**测试步骤 3:**
1.  重新编译并运行 `make qemu`。
2.  运行 `mmaptest`。
3.  现在，测试程序应该能够成功读取 `mmap` 映射的内存内容，并执行到第一个 `munmap` 调用。由于 `munmap` 还未实现，它可能会在这里失败或表现异常。这是又一个好的进展！

```shell
xv6 kernel is booting

hart 2 starting
hart 1 starting
init: starting sh
$ mmaptest
mmap_test starting
test mmap f
mmaptest: mmap_test failed: munmap (1), pid=3
$ 
```

---

### 第五步：实现 `munmap` 系统调用

`munmap` 用于解除一段内存映射。如果需要解除的区域是 `MAP_SHARED` 且已被修改，需要将内容写回文件。

**在 `kernel/sysfile.c` 中实现 `sys_munmap`：**

```c
uint64
sys_munmap(void)
{
  uint64 addr;
  int length;
  if (argaddr(0, &addr) < 0 || argint(1, &length) < 0) {
    return -1;
  }
  if (length <= 0) {
    return -1;
  }

  struct proc *p = myproc();
  addr = PGROUNDDOWN(addr); // 起始地址对齐
  length = PGROUNDUP(length); // 长度对齐

  // 1. 查找包含该地址范围的 VMA
  struct vma *vma = 0;
  int found_index = -1;
  for (int i = 0; i < NVMA; i++) {
    if (p->vmas[i].used &&
        addr >= p->vmas[i].addr &&
        addr + length <= p->vmas[i].addr + p->vmas[i].length) {
      vma = &p->vmas[i];
      found_index = i;
      break;
    }
  }
  if (vma == 0) {
    return -1; // 没有找到对应的 VMA
  }

  // 2. 如果是 MAP_SHARED 映射，尝试将脏页写回文件
  if ((vma->flags & MAP_SHARED)) {
    // 使用 filewrite 写回从 addr 开始的前 length 字节
    // filewrite 内部会处理页表遍历和拷贝，但我们只应写入已分配且脏的页。
    // 一个更精确的实现会遍历涉及的每个页，检查 PTE_D (Dirty) 位，但这里简化处理，全部写回。
    filewrite(vma->file, addr, length);
  }

  // 3. 解除用户页表映射并释放物理内存
  //    uvmunmap 的第四个参数 (do_free) 为 1 表示要释放物理页
  uvmunmap(p->pagetable, addr, length / PGSIZE, 1);

  // 4. 更新 VMA
  if (addr == vma->addr && length == vma->length) {
    // 情况 A: 完全解除整个映射
    fileclose(vma->file); // 减少文件的引用计数
    vma->used = 0;       // 标记 VMA 为空闲
  } else if (addr == vma->addr) {
    // 情况 B: 从开头解除部分映射
    vma->addr += length;
    vma->length -= length;
    vma->offset += length; // 文件偏移也需要更新！
  } else if (addr + length == vma->addr + vma->length) {
    // 情况 C: 从末尾解除部分映射
    vma->length -= length;
  } else {
    // 情况 D: 从中间解除映射（复杂，本实验可能不要求）
    // 简单处理：返回错误或杀死进程
    return -1;
  }

  // 5. 如果解除的是进程地址空间末尾的映射，更新 p->sz
  //    (这假设映射是从高地址连续分配的，可能不总是成立，但适用于简单实现)
  if (addr + length == p->sz) {
    p->sz = addr;
  }

  return 0;
}
```

*   **查找 VMA**：找到要解除的地址范围所属的 VMA。
*   **写回文件**：如果映射是 `MAP_SHARED`，调用 `filewrite` 将内存中的修改写回文件。这是一个简化实现，理想情况下应检查“脏页”（Dirty Page）。
*   **解除映射**：`uvmunmap` 函数会遍历页表，找到指定虚拟地址范围内的所有页表项，清除它们并释放对应的物理页（因为 `do_free=1`）。
*   **更新 VMA**：根据解除映射是发生在 VMA 的头部、尾部还是整体，来调整 VMA 的元数据。如果整个映射被解除，还需要关闭文件 (`fileclose`)。

**修复 `uvmunmap`：**
由于 `mmap` 的懒加载特性，`uvmunmap` 在遍历页表时可能会遇到无效（PTE_V=0）的页表项，这是正常的（这些页从未被访问过）。我们需要修改 `kernel/vm.c` 中的 `uvmunmap` 函数，使其遇到无效 PTE 时跳过而不是 panic。

```c
// kernel/vm.c
void
uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free)
{
  ...
  for(a = va; a < va + npages*PGSIZE; a += PGSIZE){
    if((pte = walk(pagetable, a, 0)) == 0)
      panic("uvmunmap: walk");
    if((*pte & PTE_V) == 0) {
      // 如果页不存在（PTE_V=0），可能是因为懒加载导致的，跳过即可
      continue;
    }
    ...
    if(do_free){
      ...
    }
    *pte = 0;
  }
}
```

**测试步骤 4:**
1.  重新编译并运行 `make qemu`。
2.  运行 `mmaptest`。
3.  `mmap_test` 测试用例现在应该能够完全通过！

```shell
init: starting sh
$ mmaptest
mmap_test starting
test mmap f
test mmap f: OK
test mmap private
test mmap private: OK
test mmap read-only
test mmap read-only: OK
test mmap read/write
test mmap read/write: OK
test mmap dirty
test mmap dirty: OK
test not-mapped unmap
test not-mapped unmap: OK
test mmap two files
test mmap two files: OK
mmap_test: ALL OK
fork_test starting
panic: uvmcopy: page not present
```

---

### 第六步：修改 `exit` 和 `fork`

进程退出或分裂时，需要正确处理其 VMA。

**1. 修改 `exit` 系统调用（`kernel/proc.c`）：**
当进程退出时，需要像调用 `munmap` 一样清理其所有映射。

```c
// kernel/proc.c

#include "fcntl.h"


void
exit(int status)
{
  ...
  // Close all open files.
  for(int fd = 0; fd < NOFILE; fd++){
    ...
  }

  // 新增：遍历并清理所有 VMA
  for (int i = 0; i < NVMA; i++) {
    if (p->vmas[i].used) {
      // 计算需要解除的页数
      uint64 npages = p->vmas[i].length / PGSIZE;
      // 如果是共享映射，写回文件
      if (p->vmas[i].flags & MAP_SHARED) {
        filewrite(p->vmas[i].file, p->vmas[i].addr, p->vmas[i].length);
      }
      // 解除页表映射并释放物理页
      uvmunmap(p->pagetable, p->vmas[i].addr, npages, 1);
      // 关闭文件
      fileclose(p->vmas[i].file);
      // 标记 VMA 为空闲
      p->vmas[i].used = 0;
    }
  }

  begin_op();
  ...
}
```

**2. 修改 `fork` 系统调用（`kernel/proc.c`）：**
当父进程 fork 子进程时，子进程需要继承父进程的 VMA 信息。

```c
// kernel/proc.c
int
fork(void)
{
  ...
  // Copy user memory from parent to child.
  if(uvmcopy(p->pagetable, np->pagetable, p->sz) < 0){
    ...
  }

  // 新增：复制父进程的 VMA 数组到子进程
  for (int i = 0; i < NVMA; i++) {
    if (p->vmas[i].used) {
      // 复制 VMA 结构体
      np->vmas[i] = p->vmas[i];
      // 增加被映射文件的引用计数！
      filedup(np->vmas[i].file);
    }
  }

  np->sz = p->sz;
  ...
}
```
**解析：**
*   `exit`: 进程退出时必须清理所有资源。这里遍历所有 VMA，执行写回（如果是共享的）、解除映射、关闭文件等操作。
*   `fork`: 子进程复制父进程的 VMA 数组。关键是调用 `filedup` 增加文件的引用计数，这样即使父进程关闭文件，子进程的映射依然有效。注意，子进程的页表是通过 `uvmcopy` 复制的，但 `uvmcopy` 会遇到懒加载留下的无效 PTE，我们需要同样修改它。

**修复 `uvmcopy`（`kernel/vm.c`）：**
和 `uvmunmap` 类似，复制页表时遇到无效 PTE 是正常的，应跳过。

```c
// kernel/vm.c
int
uvmcopy(pagetable_t old, pagetable_t new, uint64 sz)
{
  ...
  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walk(old, i, 0)) == 0)
      panic("uvmcopy: pte should exist");
    if((*pte & PTE_V) == 0) {
      // 如果页不存在（PTE_V=0），可能是因为懒加载导致的，跳过即可
      continue;
    }
    ...
  }
  ...
}
```

**测试步骤 5:**
1.  重新编译并运行 `make qemu`。
2.  运行 `mmaptest`。现在 `fork_test` 和 `mmap_test` 都应该能通过！
3.  **强烈建议**：运行 `usertests` 以确保你的修改没有破坏其他内核功能。

---

### 最终测试

在 xv6 中运行命令：
```bash
$ mmaptest
```
你应该看到：
```
mmap_test starting
test mmap f
test mmap f: OK
test mmap private
test mmap private: OK
test mmap read-only
test mmap read-only: OK
test mmap read/write
test mmap read/write: OK
test mmap dirty
test mmap dirty: OK
test not-mapped unmap
test not-mapped unmap: OK
test mmap two files
test mmap two files: OK
mmap_test: ALL OK
fork_test starting
fork_test OK
mmaptest: all tests succeeded
```

然后运行：
```bash
$ usertests
```
`usertests` 也应该全部通过。

```shell
make grade
```

![alt text](<Pasted image 20250904192036.png>)


**补充：**
编译 xv6 时遇到未声明错误，通常是因为相关头文件未包含或宏定义缺失，以及有时**头文件依赖关系**可能导致问题。检查：所有相关文件中的**头文件包含顺序**