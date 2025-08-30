# 一、准备工作

1. 获取代码、切到 lab 分支，并清理：
    

```
git checkout traps
git checkout -b traps_test
make clean
```

---
# 二、总体三部分

1. **RISC-V Assembly（理解题，easy）**
    
2. **Backtrace（中等）** — 在内核实现 `backtrace()` 并在 `sys_sleep` 中调用，运行 `bttest`。
    
3. **Alarm（hard）** — 新增 `sigalarm` / `sigreturn` 两个系统调用，修改 `proc`、`usertrap`、`allocproc/freeproc`、syscall 表等，运行 `alarmtest` & `usertests`。
    

---

# 三、实验流程
## 第一部分：RISC-V assembly

目标：读懂 `user/call.c` 编译出的 `user/call.asm`，回答 lab 页面里的问题（哪个寄存器传参、哪里调用、printf 地址、内存字节序等）。

1. 生成用户汇编：
```
make fs.img    # 会生成用户程序的 .asm（例如 user/call.asm）
```

2. 打开 `user/call.asm` 看函数 `g`, `f`, `main`。注意：
    
    - RISC-V 函数参数按顺序放在 `a0, a1, a2, ...`（所以 13 存在 a2 之类的问题就是照这个规则）。这是课程和手册的常识。
        
    - 用 `riscv64-unknown-elf-objdump -d user/call` 或直接查看 `user/call.asm`，找 `jal` / `jalr` 指令查看调用位置、`auipc`+`jalr` 常用于跳转到 `printf`（高位地址由 `auipc` 设，再用 `jalr` 跳）。
        

> 这部分主要读懂 `call.asm`。

---

## 第二部分：Backtrace

目标：在内核实现 `backtrace()`，并在 `sys_sleep` 中调用；运行 `bttest` 得到调用链地址列表。核心原理：xv6 用 s0（frame pointer）保存当前帧指针；在每一帧里返回地址位于 `fp - 8`，上一个帧指针位于 `fp - 16`。循环直到 `fp` 不再位于单页内（xv6 内核栈每进程一页）
  
### 1. 在 `kernel/riscv.h` 增加读取帧指针函数

在 `kernel/riscv.h` 新增：

```c
// kernel/riscv.h
static inline uint64
r_fp(void)
{
  uint64 x;
  asm volatile("mv %0, s0" : "=r" (x));
  return x;
}
```

（用途：从 C 读取 `s0` 寄存器，即当前函数帧指针）

### 2. 在 `kernel/defs.h` 声明原型

在 `kernel/defs.h`）添加：

```c
// kernel/defs.h
// printf.c
void            backtrace(void);
```
（这样 sys_sleep 能调用）

### 3. 在 `kernel/printf.c` 实现 `backtrace()`

打开 `kernel/printf.c`在适当位置（例如 printf 实现后或者 panic 前）添加：

```c
// kernel/printf.c

#include "riscv.h"   // 应该已经有了，已有的话可忽略

void
backtrace(void)
{
  uint64 fp = r_fp();   // 当前帧指针（s0）
  printf("backtrace:\n");

  // 每个内核栈是一页，判断 fp 是否仍在该页内
  while (PGROUNDUP(fp) - PGROUNDDOWN(fp) == PGSIZE) {
    // 返回地址保存在 fp - 8
    uint64 ret = *(uint64 *)(fp - 8);
    printf("%p\n", ret);
    // 上一个帧指针保存在 fp - 16
    fp = *(uint64 *)(fp - 16);
    if (fp == 0) // 防御性检查
      break;
  }
}
```

> 说明：上面的偏移（-8 / -16）来自 gcc 生成的栈帧约定（lab 提示亦如是）。若你想把输出换成函数名+行号，是可选挑战（需要 addr2line / ELF 符号解析）。

### 4. 在 `kernel/sysproc.c` 的 `sys_sleep` 中调用 `backtrace()`

找到 `sys_sleep` 的实现（一般在 `kernel/sysproc.c` 中），在合适位置加入 `backtrace();`用于 bttest。例如：

```c
// kernel/sysproc.c
uint64
sys_sleep(void)
{
  int n;
  if (argint(0, &n) < 0)
    return -1;

  // 在睡前打印回溯（用于测试）
  backtrace();

  // 原有 sleep 逻辑...
  // ...
}
```

> 这样运行 `bttest` 时会显示 `backtrace:` + 地址列表（实验要求）。随后可用 `riscv64-unknown-elf-addr2line -e kernel/kernel` 把地址转到源码文件行号。
### 编译与测试 backtrace

```
make clean
make
make qemu               # 或 make qemu-nox
# 在 xv6 shell 中运行：
$ bttest
# 看到类似：
backtrace:
0x0000000080002cf0
0x0000000080002bb6
0x00000000800028a0
```

（地址可能不同）可以在宿主机运行 `riscv64-unknown-elf-addr2line -e kernel/kernel <addr>` 输入地址得到源码位置。

---

## 第三部分：Alarm（实现 sigalarm / sigreturn）

任务：增加两个系统调用 `sigalarm(int ticks, void (*handler)())` 与 `sigreturn()`。当某进程在用户态运行并消耗了指定数量的 ticks（timer 中断次数）后，内核会在返回用户态时把用户的 `epc` 改为 `handler`，并把**被中断时的完整 trapframe**保存起来；`handler` 执行完后应调用 `sigreturn()` 恢复原来被中断时的 trapframe，继续原来执行流。

实现要点）：

- **先保存当前 `p->trapframe` 到 `p->alarm_trapframe`，再修改 `p->trapframe->epc` 指向 handler**（如果顺序反了会丢失被中断时的 PC）。
- 需要防止 handler 执行期间再次被中断触发（加 `is_alarming` / `alarm_on` 标志）。
- `alarm_trapframe` 应用 `kalloc()` 分配（和 `trapframe` 一样）。
- `sigreturn()` 要 `memmove` 恢复 `p->trapframe`，并把 `is_alarming` 置 0。

### 1) 在 `user/user.h` 声明用户接口

在 `user/user.h`（用户头文件）中加入：

```c
// user/user.h
int sigalarm(int ticks, void (*handler)());
int sigreturn(void);
```

（这样用户程序 `alarmtest.c` 能编译链接）

### 2) 生成用户 syscall stub（修改 `user/usys.pl`）

在 `user/usys.pl`（生成 `user/usys.S` 的脚本）中增加：

```perl
entry("sigalarm");
entry("sigreturn");
```

然后 `make` 时会自动生成 `user/usys.S` 中的 stub，让用户程序能把参数传给内核。

### 3) 在内核 syscall 编号中注册（`kernel/syscall.h` / `kernel/syscall.c`）

打开 `kernel/syscall.h`，在现有 `SYS_*` 定义末尾添加两行（**请把数字设置为当前列表中下一号**；多数 xv6 版本 `SYS_close` 是 21，所以下一号通常为 22）：

```c
// kernel/syscall.h
#define SYS_sigalarm  22
#define SYS_sigreturn 23
```

在 `kernel/syscall.c` 中：

1. 在文件中 extern 列表中加入：
        
```c
extern uint64 sys_sigalarm(void);
extern uint64 sys_sigreturn(void);
```

2. 在 `static uint64 (*syscalls[])(void) = { ... }` 数组中，添加：
    
```c
[SYS_sigalarm]  sys_sigalarm,
[SYS_sigreturn] sys_sigreturn,
```

（这样内核知道 syscall 编号对应内核实现）

### 4) 在 `kernel/proc.h` 扩展 `struct proc`

编辑 `kernel/proc.h` 中 `struct proc`，在合适位置（比如紧跟 `struct trapframe *trapframe;` 后）添加：

```c
// kernel/proc.h (在 struct proc 内)
int alarm_interval;            // 报警间隔（ticks），0 表示不启用
void (*alarm_handler)();       // 用户空间处理函数地址
int ticks_count;               // 已走过的 ticks 计数
int is_alarming;               // handler 是否正在执行（防重入）
struct trapframe *alarm_trapframe; // 用来保存被中断时的 trapframe（以便 sigreturn 恢复）
```

> 解释：`alarm_trapframe` 用 `kalloc()` 分配一页（与 `trapframe` 一样），保存当时的寄存器上下文；`is_alarming` 阻止 handler 执行时再次触发（test2 要求）。

### 5) 在 `kernel/proc.c` 的 `allocproc()` 与 `freeproc()` 做初始化和释放

找到 `allocproc()`（通常开头分配 `p->trapframe = (struct trapframe*)kalloc()` 的那段），在分配 `trapframe` 后添加：

```c
// kernel/proc.c, inside allocproc() after allocating p->trapframe
if ((p->alarm_trapframe = (struct trapframe*)kalloc()) == 0) {
  freeproc(p);
  release(&p->lock);
  return 0;
}
p->is_alarming = 0;
p->alarm_interval = 0;
p->alarm_handler = 0;
p->ticks_count = 0;
```

并在 `freeproc()`（释放 proc 的地方）添加释放代码：

```c
// kernel/proc.c, inside freeproc()
if (p->alarm_trapframe)
  kfree((void*)p->alarm_trapframe);
p->alarm_trapframe = 0;
p->is_alarming = 0;
p->alarm_interval = 0;
p->alarm_handler = 0;
p->ticks_count = 0;
```

> 注意：`allocproc()` 的错误回滚路径必须正确处理 `alarm_trapframe`，上面示例用 `freeproc` 在失败时回退。

### 6) 在 `kernel/sysproc.c` 实现 `sys_sigalarm` 和 `sys_sigreturn`

在 `kernel/sysproc.c` 文件末尾，在其它 `sys_*` 函数附近加入：

```c
// kernel/sysproc.c

uint64
sys_sigalarm(void)
{
  int interval;
  uint64 handler;   // address

  if (argint(0, &interval) < 0)
    return -1;
  if (argaddr(1, &handler) < 0)
    return -1;

  struct proc *p = myproc();

  p->alarm_interval = interval;
  p->alarm_handler = (void(*)())handler;
  p->ticks_count = 0;
  // is_alarming already initialized in allocproc
  return 0;
}

uint64
sys_sigreturn(void)
{
  struct proc *p = myproc();
  // 恢复保存的 trapframe（恢复寄存器、epc 等）
  memmove(p->trapframe, p->alarm_trapframe, sizeof(struct trapframe));
  p->is_alarming = 0;
  return 0;
}

```

>`argaddr` / `argint` 用来取用户传入的 syscall 参数。`sys_sigreturn` 把保存的上下文拷回 `p->trapframe`，usertrapret 会使用 `p->trapframe` 的 `epc` 恢复返回点。

### 7) 在 `kernel/trap.c` 的 `usertrap()` 中，在 timer 中断处加入触发逻辑

打开 `kernel/trap.c`，找到 `usertrap()` 并定位处理 timer 中断的分支（通常是 `if (which_dev == 2) { ... }`，`which_dev==2` 表示 timer interrupt）。在该分支处加入如下逻辑（**重要：先保存 p->trapframe，再修改 p->trapframe->epc**）：

```c
// kernel/trap.c inside usertrap(), in timer branch
if (which_dev == 2) {
  // 原有的时钟处理例如 ticks++ wakeup(&ticks) 等保留
  // 然后，处理进程级别的 alarm（如果该 trap 是用户态的进程）
  struct proc *p = myproc();
  if (p != 0 && p->alarm_interval > 0) {
    p->ticks_count++;
    if (p->ticks_count >= p->alarm_interval && p->is_alarming == 0) {
      // 保存当前用户 trapframe（寄存器/epc 等）
      memmove(p->alarm_trapframe, p->trapframe, sizeof(struct trapframe));
      // 修改 trapframe->epc：返回用户时跳转到 handler
      p->trapframe->epc = (uint64)p->alarm_handler;
      p->ticks_count = 0;
      p->is_alarming = 1;
    }
  }
  yield();
}
```

> **顺序要严格**：先 `memmove` 保存当前上下文（包含原来的 `epc`），再把 `p->trapframe->epc` 改为 handler 地址。否则 `alarm_trapframe` 会保存错误的 `epc`。

### 8) Makefile / UPROGS 增加 `_alarmtest`

在顶层 `Makefile`（或仓库里保持用户程序列表的地方），找到 `UPROGS` 变量，把 `_alarmtest\`（注意 xv6 的约定前缀 `_`）加入。例如：

```makefile
# Makefile
UPROGS=\
  _cat\
  _echo\
  ...
  _alarmtest\
```

### 9) 重新编译并测试 alarm 功能

完整流程：

```bash
make clean
make          # 编译 kernel + user 程序
make qemu     # 启动 xv6（可用 make qemu-nox 或 make CPUS=1 qemu-gdb 方便调试）
```

在 xv6 shell 中运行：

```
$ alarmtest
test0 start
.........................alarm!
test0 passed
test1 start
..alarm!
..alarm!
..alarm!
...alarm!
...alarm!
..alarm!
..alarm!
...alarm!
...alarm!
..alarm!
test1 passed
test2 start
..........................alarm!
test2 passed
$
```

接着运行 `usertests` 确保没有回归：`usertests` 应显示 `ALL TESTS PASSED`。

---

# 四、整体测试

添加 `time.txt` 记录实验用时，`answers-traps.txt`回答实验第一部分的提问。

```
make grade
```

![alt text](<Pasted image 20250829032919.png>)


实验成功合并到主分支

```bash
git checkout traps       # 回到主分支
git merge traps_test    # 合并实验分支的改动
```

- 如果有冲突，Git 会提示手动解决
- 合并后实验改动就永久保存在分支 `traps`
    

删除测试分支

如果确认 traps_test 分支的改动已经成功合并到主分支 `traps` 且不再需要该分支时，可以安全地删除它。

删除本地分支的命令：

```bash
git checkout traps 
git branch -d traps_test
```

如果需要同时删除远程仓库（比如 GitHub 上）的 `pgtbl_test` 分支，可以执行：
```bash
git push github --delete traps_test
```

