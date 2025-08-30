

# 一、前言

实验目标：为 xv6 操作系统实现**延迟分配（Lazy Allocation）** 机制。目前，当用户程序调用 `sbrk()` 系统调用请求更多内存时，xv6 会立即分配物理内存并建立页表映射。这种**急切分配（Eager Allocation）** 策略在请求大量内存时（如 1GB）会产生显著开销，因为需要分配和映射大量页面（1GB/4KB=262,144 个页面）。

延迟分配策略优化了这一过程：`sbrk()` 只记录分配了哪些用户地址（增大 `p->sz`），但并不立即分配物理内存，也不建立有效的页表映射（实际上会将相关页表条目标记为无效）。当进程首次尝试访问这些延迟分配的内存时，会触发**页面错误（Page Fault）**，此时内核再分配物理内存、清零并建立映射，最后让进程继续执行。

实验包含三个主要部分：
1.  **修改 `sbrk()` 系统调用**，消除其中的内存分配。
2.  **修改陷阱处理代码**，响应页面错误，实际分配和映射内存。
3.  **处理边界情况**，确保通过所有测试。

实验准备：进入 xv6-labs-2020 的代码目录中，并切换到 `lazy` 分支：
```bash
$ git checkout lazy
$ make clean
```

---
## 二、实验记录
## Part1：Eliminate allocation from sbrk() (easy)

**目标**：修改 `sys_sbrk()`，使其不再立即分配物理内存，仅增加进程的 `sz` 字段。

**修改位置**：`kernel/sysproc.c` 文件中的 `sys_sbrk()` 函数。

**步骤**：
1.  找到 `sys_sbrk()` 函数。
2.  注释掉或删除对 `growproc(n)` 的调用。
3.  添加逻辑，仅增加进程的 `sz` 字段（`myproc()->sz`）。
4.  需要处理参数 `n` 为负数的情况（缩小内存），这时需要立即调用 `uvmdealloc` 来释放内存。

**代码**：
```c
uint64
sys_sbrk(void)
{
  int addr;
  int n;
  struct proc *p = myproc(); // 获取当前进程

  if(argint(0, &n) < 0)
    return -1;
  addr = p->sz; // 保存旧的进程大小

  // 删除原有的 growproc() 调用
  // if(growproc(n) < 0)
  //   return -1;

  if (n > 0) {
    // 延迟分配：只增加进程的虚拟地址空间大小
    p->sz += n;
  } else if (p->sz + n > 0) {
    // 如果 n 是负数，且收缩后大小仍大于0，则立即释放内存
    p->sz = uvmdealloc(p->pagetable, p->sz, p->sz + n);
  } else {
    // 如果收缩到小于0，则返回错误
    return -1;
  }
  return addr; // 返回旧的进程大小
}
```

**验证**：完成修改后，编译并运行 xv6 (`make qemu`)，在 shell 中输入 `echo hi`。可以看到类似下面的错误信息，这是因为 `sbrk()` 不再分配物理内存，访问新分配的内存地址会触发页面错误，而内核尚未处理这种错误：

```
init: starting sh
$ echo hi
usertrap(): unexpected scause 0x000000000000000f pid=3
            sepc=0x0000000000001258 stval=0x0000000000004008
va=0x0000000000004000 pte=0x0000000000000000
panic: uvmunmap: not mapped
```
*   `scause 0xf` (15) 表示存储指令导致的页面错误（Store Page Fault）。
*   `stval 0x4008` 表示引发错误的虚拟地址。
*   `sepc` 指向用户程序中引发错误的指令地址。
*   最后的 `panic` 发生在后续操作（可能是进程退出释放内存时）中，因为页表映射不存在。我们将在后续步骤中修复这些问题。

---

## Part2：Lazy allocation (moderate)

**目标**：修改陷阱处理逻辑，当用户空间发生页面错误时，分配物理内存并建立映射，然后返回到用户空间继续执行。

**修改位置 1**：`kernel/trap.c` 文件中的 `usertrap()` 函数。

**步骤**：
1.  在 `usertrap()` 中，添加一个 `else if` 分支来处理页面错误。
2.  使用 `r_scause()` 判断中断原因，13 表示加载页面错误（Load Page Fault），15 表示存储页面错误（Store Page Fault）。
3.  使用 `r_stval()` 获取引发页面错误的虚拟地址。
4.  调用 `kalloc()` 分配一个物理页。
5.  如果分配失败，杀死当前进程。
6.  检查引发错误的虚拟地址是否合法（在堆的增长范围内且不在用户栈之下）。
7.  使用 `mappages()` 将物理页映射到用户的虚拟地址（需要将地址向下舍入到页面边界 `PGROUNDDOWN(va)`）。
8.  如果映射失败，释放分配的物理页并杀死进程。

**代码**（修改 `kernel/trap.c` 中的 `usertrap()` 函数）：
```c
// ...
#include "spinlock.h"
#include "proc.h"
// ...

void
usertrap(void)
{
  // ... 其他代码保持不变 ...
  } else if((which_dev = devintr()) != 0){
    // ok
  } else if (r_scause() == 13 || r_scause() == 15) {
    // 处理页面错误 (13: load, 15: store)
    uint64 va = r_stval();  // 获取引发错误的虚拟地址
    struct proc *p = myproc();
    uint64 ka;

    // 检查错误地址的合法性：
    // 1. 不能在进程已分配的内存范围之外 (va >= p->sz)
    // 2. 不能在用户栈之下（包括栈保护页guard page）
    if (va >= p->sz || va <= PGROUNDDOWN(p->trapframe->sp)) {
      p->killed = 1;
      goto end;
    }

    // 分配物理页面
    ka = (uint64) kalloc();
    if (ka == 0) {
      // 物理内存耗尽
      p->killed = 1;
      goto end;
    }

    // 初始化物理页面为0
    memset((void*)ka, 0, PGSIZE);

    // 将虚拟地址向下舍入到页面边界并建立映射
    va = PGROUNDDOWN(va);
    if (mappages(p->pagetable, va, PGSIZE, ka, PTE_W | PTE_R | PTE_U) != 0) {
      // 映射失败，释放物理页面并杀死进程
      kfree((void*)ka);
      p->killed = 1;
    }
  } else {
    // 其他未知陷阱原因，保持原有的错误处理和打印
    printf("usertrap(): unexpected scause %p pid=%d\n", r_scause(), p->pid);
    printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
    p->killed = 1;
  }
 end:
  // ... 后续代码保持不变 ...
}
```

**修改位置 2**：`kernel/vm.c` 文件中的 `uvmunmap()` 函数。

**问题**：当进程退出或调用 `sbrk` 缩小内存时，会调用 `uvmunmap` 来解除映射并释放物理内存。由于延迟分配，一些虚拟地址范围可能根本没有对应的物理页和有效映射。原来的 `uvmunmap` 遇到无映射或映射无效的情况会 `panic`，现在需要修改为跳过这些页面。

**步骤**：
1.  找到 `uvmunmap` 函数。
2.  修改其中两个 `panic("uvmunmap: walk")` 和 `panic("uvmunmap: not mapped")` 的地方，改为 `continue` 跳过该页面。

**代码**（修改 `kernel/vm.c` 中的 `uvmunmap()` 函数）：
```c
void
uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free)
{
  // ... 前面的代码保持不变 ...
  for(a = va; a < va + npages * PGSIZE; a += PGSIZE){
    if((pte = walk(pagetable, a, 0)) == 0)
      // panic("uvmunmap: walk"); // 原来会panic
      continue; // 改为跳过该页面
    if((*pte & PTE_V) == 0)
      // panic("uvmunmap: not mapped"); // 原来会panic
      continue; // 改为跳过该页面
    if(PTE_FLAGS(*pte) == PTE_V)
      panic("uvmunmap: not a leaf");
    if(do_free){
      uint64 pa = PTE2PA(*pte);
      kfree((void*)pa);
    }
    *pte = 0;
  }
}
```

**验证**：完成以上修改后，再次编译运行 xv6 (`make qemu`)。现在在 shell 中输入 `echo hi` 应该能够正常工作：
```
init: starting sh
$ echo hi
hi
$
```
这表明内核已经能够处理由于延迟分配引起的页面错误。

---

## Part3：Lazytests and Usertests (moderate)

**目标**：通过提供的 `lazytests` 和 `usertests` 测试程序，确保延迟分配器在各种边界情况下都能正确工作。

**需要处理的边界情况**：
1.  **sbrk() 参数为负数**：在 `sys_sbrk()` 中已经处理。
2.  **访问超出 sbrk() 分配范围的地址**：在 `usertrap()` 的页面错误处理中，通过 `if (va >= p->sz ...)` 检查，杀死进程。
3.  **在 fork() 中正确处理父进程到子进程的内存拷贝**：修改 `uvmcopy()`。
4.  **处理进程向 read/write 等系统调用传递有效但尚未分配内存的地址**：修改 `walkaddr()`。
5.  **处理内存不足（kalloc() 失败）**：在 `usertrap()` 的页面错误处理中已经检查 `kalloc()` 的返回值。
6.  **处理用户栈之下发生的页面错误**：在 `usertrap()` 的页面错误处理中，通过 `if (... || va <= PGROUNDDOWN(p->trapframe->sp))` 检查，杀死进程。

**修改位置 1**：`kernel/vm.c` 文件中的 `uvmcopy()` 函数（处理 fork）。

**问题**：`fork` 会调用 `uvmcopy` 复制父进程的页表给子进程。由于父进程可能存在延迟分配的页面（PTE_V 无效），复制时需要跳过这些页面，而不是 panic。

**步骤**：
1.  找到 `uvmcopy` 函数。
2.  修改其中两个 `panic("uvmcopy: pte should exist")` 和 `panic("uvmcopy: page not present")` 的地方，改为 `continue`。

**代码**（修改 `kernel/vm.c` 中的 `uvmcopy()` 函数）：
```c
int
uvmcopy(pagetable_t old, pagetable_t new, uint64 sz)
{
  // ... 前面的代码保持不变 ...
  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walk(old, i, 0)) == 0)
      // panic("uvmcopy: pte should exist"); // 原来会panic
      continue; // 改为跳过该页面
    if((*pte & PTE_V) == 0)
      // panic("uvmcopy: page not present"); // 原来会panic
      continue; // 改为跳过该页面
    // ... 后面的代码保持不变 ...
    }
  }
  // ... 后面的代码保持不变 ...
}
```

**修改位置 2**：`kernel/vm.c` 文件中的 `walkaddr()` 函数（处理系统调用）。

**问题**：当用户程序调用 `read`、`write` 等系统调用时，内核需要访问用户提供的缓冲区地址。如果该缓冲区地址位于延迟分配的区域且尚未分配物理页，`walkaddr` 会返回 0，导致系统调用失败。我们需要让 `walkaddr` 能像页面错误处理程序一样，必要时分配物理页。

**步骤**：
1.  找到 `walkaddr` 函数。
2.  在检查 `pte` 是否存在或有效之前，先检查地址 `va` 是否合法（参考 `usertrap` 中的检查）。
3.  如果 `pte` 不存在或无效，但 `va` 是合法的延迟分配地址，则分配物理页并建立映射。
4.  需要包含 `proc.h` 和 `spinlock.h` 以使用 `myproc()`。

**代码**（修改 `kernel/vm.c` 中的 `walkaddr()` 函数）：
```c
// 确保在文件开头包含了必要的头文件
#include "spinlock.h"
#include "proc.h"

uint64
walkaddr(pagetable_t pagetable, uint64 va)
{
  pte_t *pte;
  uint64 pa;
  struct proc *p = myproc(); // 获取当前进程

  if(va >= MAXVA)
    return 0;

  pte = walk(pagetable, va, 0);

  // 如果页表项不存在或无效，但地址在进程的堆空间内且不在栈下，则尝试分配内存
  if(pte == 0 || (*pte & PTE_V) == 0) {
    if (va >= p->sz || va <= PGROUNDDOWN(p->trapframe->sp)) {
      return 0; // 非法地址
    }
    // 模拟页面错误处理程序：分配并映射页面
    uint64 ka = (uint64)kalloc();
    if(ka == 0) {
      return 0; // 分配失败
    }
    memset((void*)ka, 0, PGSIZE);
    va = PGROUNDDOWN(va);
    if(mappages(pagetable, va, PGSIZE, ka, PTE_W|PTE_R|PTE_U) != 0) {
      kfree((void*)ka);
      return 0;
    }
    // 映射成功后，重新获取pte
    pte = walk(pagetable, va, 0); // 或者直接计算物理地址 ka | (va & (PGSIZE-1))
    if(pte == 0) {
        return 0; // 理论上不会发生
    }
    // 注意：我们映射了整个页，所以需要返回该页内正确偏移的物理地址
    pa = ka + (va & (PGSIZE-1)); // 计算实际物理地址
    return pa;
  }

  // 原来的检查和处理
  if((*pte & PTE_V) == 0)
    return 0;
  if((*pte & PTE_U) == 0)
    return 0;
  pa = PTE2PA(*pte);
  // ... 后续的权限检查等保持不变 ...
  return pa + (va & (PGSIZE-1)); // 返回对应的物理地址
}
```
**注意**：上面的 `walkaddr` 修改是一种方法。另一种更简洁的方法是让 `walkaddr` 只做地址转换，而在系统调用路径的上层（例如 `copyin`、`copyout` 或其调用者）处理页面错误。但根据实验提示，修改 `walkaddr` 是可行的方案。

**最终验证**：
完成所有修改后，重新编译 xv6 (`make clean && make qemu`)。
1.  首先运行 `lazytests`：
    ```bash
    $ lazytests
	lazytests starting
	running test lazy alloc
	test lazy alloc: OK
	running test lazy unmap
	test lazy unmap: OK
	running test out of memory
	test out of memory: OK
	ALL TESTS PASSED
	$ 
    ```
    
2.  然后运行 `usertests`：
    ```bash
    $ usertests
    ... (会输出大量测试结果)
    ALL TESTS PASSED
    ```
    
3. 最后可以 `make grade`整体测试

![alt text](<Pasted image 20250830025256.png>)

---

# 三、总结

通过修改 xv6 内核，实现了对用户堆内存的延迟分配：
1.  **`sys_sbrk()`**：只更新进程大小 `p->sz`，推迟实际的内存分配。
2.  **`usertrap()`**：捕获页面错误（13 或 15），在错误地址合法时分配物理内存并建立映射。
3.  **`uvmunmap()` 和 `uvmcopy()`**：适应延迟分配，跳过未实际映射的页面。
4.  **`walkaddr()`**：确保系统调用能访问延迟分配的内存。
5.  **各种边界检查**：确保程序的非法访问能被正确捕获和处理。

延迟分配优化了内存分配的开销，将其平摊到实际访问内存的时刻，避免了分配大量内存但只使用少量所带来的浪费。