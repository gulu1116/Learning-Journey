# 一、获取并切到 syscall 分支

进入lab的仓库，切到 **syscall** 分支并清理构建：

```bash
cd ~/6.s081-xv6-labs       # 你的路径
git fetch
git checkout syscall
make clean
```

# 二、任务一：实现 `trace` 系统调用（系统调用回传时打印）

目标：为当前进程（及其后续 fork 的子孙）开启“按位掩码”的 syscall 追踪；当匹配的系统调用**返回**时，打印：

```
<pid>: syscall <name> -> <return-value>
```

并通过提供的 `user/trace.c` 来运行其它程序时开启追踪。([MIT CSAIL PDOS](https://pdos.csail.mit.edu/6.S081/2023/labs/syscall.html "Lab: System calls"))

> 总体改动：  
> **用户态**：在 `user/user.h` 声明、在 `user/usys.pl` 生成 stub；Makefile 确认把 `_trace` 编进镜像。  
> **内核态**：分配 syscall 编号、在 `kernel/sysproc.c` 实现 `sys_trace`；在 `struct proc` 里存 tracemask；`fork()` 继承；`kernel/syscall.c` 在调用表和打印逻辑里接入。

### 2.1 给 `trace` 分配 syscall 编号

编辑 `kernel/syscall.h`，在现有最后一个 `SYS_xxx` 后面加一行（实际编号以你文件里最后一个为准；常见是在 `SYS_close` 之后）：

```c
#define SYS_trace  22   // 若前面最后一个是 21，则这里用 22；保持递增即可
```

### 2.2 生成用户态 stub + 原型

- `user/user.h` 里添加原型：
    
```c
int trace(int);
```

- `user/usys.pl` 里加入一行（和其它 entry 同格式）：
    
```perl
entry("trace");
```

- 确认 `Makefile` 的 `UPROGS` 里包含了 `$U/_trace`（没有就加上）：
    
```make
UPROGS= ... $U/_trace ...
```

此时 `make qemu` 应该能编译 `user/trace.c` 了，但跑起来仍不会打印，因为内核端还没实现。

### 2.3 在进程结构体里保存追踪掩码

编辑 `kernel/proc.h`，在 `struct proc` 中加一个字段：

```c
int tracemask;  // 每一位对应一个系统调用编号
```

### 2.4 实现内核处理函数 `sys_trace`

在 `kernel/sysproc.c` 里，仿照其它 `sys_*`，添加：

```c
uint64
sys_trace(void)
{
  int mask;
  if(argint(0, &mask) < 0)
    return -1;
  struct proc *p = myproc();
  p->tracemask = mask;
  return 0;
}
```

### 2.5 接入调度表和“返回时打印”

编辑 `kernel/syscall.c`：

1. 声明并把 `sys_trace` 放进系统调用表：
    
```c
extern uint64 sys_trace(void);

static uint64 (*syscalls[])(void) = {
[SYS_fork]   sys_fork,
...
[SYS_close]  sys_close,
[SYS_trace]  sys_trace,
};
```

2. 准备**名字表**用于打印（只需覆盖到实验里会用到的；完整也可以）：
    
```c
static char *syscall_names[] = {
[SYS_fork]  "fork",
[SYS_exit]  "exit",
[SYS_wait]  "wait",
[SYS_pipe]  "pipe",
[SYS_read]  "read",
[SYS_kill]  "kill",
[SYS_exec]  "exec",
[SYS_fstat] "fstat",
[SYS_chdir] "chdir",
[SYS_dup]   "dup",
[SYS_getpid]"getpid",
[SYS_sbrk]  "sbrk",
[SYS_sleep] "sleep",
[SYS_uptime]"uptime",
[SYS_open]  "open",
[SYS_write] "write",
[SYS_mknod] "mknod",
[SYS_unlink]"unlink",
[SYS_link]  "link",
[SYS_mkdir] "mkdir",
[SYS_close] "close",
[SYS_trace] "trace",
};
```

3. 在 `syscall()` 函数里，**调用后**根据掩码判断并打印（保留原有逻辑，仅在成功路径加打印）：
    
```c
void
syscall(void)
{
  struct proc *p = myproc();
  int num = p->trapframe->a7;
  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    uint64 r = syscalls[num]();
    p->trapframe->a0 = r;
    if(p->tracemask & (1 << num)) {
      // 只要求打印 pid、名字、返回值；参数不必打印
      printf("%d: syscall %s -> %d\n", p->pid, syscall_names[num], (int)r);
    }
  } else {
    printf("%d %s: unknown sys call %d\n", p->pid, p->name, num);
    p->trapframe->a0 = -1;
  }
}
```

### 2.6 `fork()` 时继承追踪掩码

编辑 `kernel/proc.c` 的 `fork()`，在拷贝完 `np->pid` 等字段后，补一行：

```c
np->tracemask = p->tracemask;   // 继承父进程的追踪设置
```

### 2.7 快速自测

启动 qemu：

```bash
make qemu
```

在 xv6 shell 内运行（与官方示例一致）：

```
$ trace 32 grep hello README
$ trace 2147483647 grep hello README
$ grep hello README
$ trace 2 usertests forkforkfork
```

输出形如官方所示即通过（pid 可能不同）。

![alt text](<Pasted image 20250815150535.png>)

![alt text](<Pasted image 20250815150551.png>)

---

# 三、任务二：实现 `sysinfo`（查询空闲内存 & 进程数）

目标：新增系统调用

```c
int sysinfo(struct sysinfo *info);
```

把内核统计结果拷回用户给的结构体：

- `freemem` = 以**字节**计的空闲物理内存；
- `nproc` = `state != UNUSED` 的进程数量。  
    通过 `user/sysinfotest.c` 验证（打印 “sysinfotest: OK” 即通过）。

> 总体改动清单：  
> **共享头文件**：新增 `kernel/sysinfo.h` 定义结构体（用户/内核都会 include 这个路径）。  
> **用户态**：`user/user.h` 前置声明 + 原型、`user/usys.pl` 生成 stub；把 `_sysinfotest` 加进 `UPROGS`。  
> **内核态**：分配 `SYS_sysinfo` 编号、在 `kalloc.c` 写统计空闲内存的函数、在 `proc.c` 写统计进程数的函数、在 `sysproc.c` 实现 `sys_sysinfo` 并 `copyout`。

### 3.1 结构体（共享头）

新建文件 `kernel/sysinfo.h`：

```c
// kernel/sysinfo.h
struct sysinfo {
  uint64 freemem;
  uint64 nproc;
};
```

### 3.2 syscall 编号 + 用户 stub

- `kernel/syscall.h` 追加：
    
```c
#define SYS_sysinfo  23   // 紧随 SYS_trace 之后；确保不与现有冲突
```

- `user/user.h` 里**先前置声明**再写原型（因为 user 也要 include 这个 struct）：
    
```c
struct sysinfo;
int sysinfo(struct sysinfo *);
```

- `user/usys.pl` 里添加：
    
```perl
entry("sysinfo");
```

- `Makefile` 的 `UPROGS` 中确保有 `$U/_sysinfotest`（没有就添加）。

### 3.3 统计空闲内存（`kalloc.c`）

在 `kernel/kalloc.c` 末尾添加一个函数（持锁遍历空闲链表，页数×PGSIZE）：

```c
uint64
freeram(void)
{
  acquire(&kmem.lock);
  struct run *r = kmem.freelist;
  uint64 n = 0;
  while(r){
    n++;
    r = r->next;
  }
  release(&kmem.lock);
  return n * PGSIZE;
}
```

并在 `kernel/defs.h` 里声明：

```c
uint64 freeram(void);
```

### 3.4 统计进程数（`proc.c`）

在 `kernel/proc.c` 末尾添加：

```c
int
countproc(void)
{
  int n = 0;
  struct proc *p;
  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->state != UNUSED)
      n++;
    release(&p->lock);
  }
  return n;
}
```

并在 `kernel/defs.h` 里声明：

```c
int countproc(void);
```

### 3.5 实现 `sys_sysinfo`（`sysproc.c`）

```c
uint64
sys_sysinfo(void)
{
  uint64 uaddr;
  if(argaddr(0, &uaddr) < 0)
    return -1;

  struct sysinfo si;
  si.freemem = freeram();
  si.nproc   = countproc();

  struct proc *p = myproc();
  if(copyout(p->pagetable, uaddr, (char*)&si, sizeof(si)) < 0)
    return -1;
  return 0;
}
```

在 `kernel/syscall.c`：

```c
extern uint64 sys_sysinfo(void);

static uint64 (*syscalls[])(void) = {
  ...
  [SYS_sysinfo] sys_sysinfo,
};
```

在名字表里可选加入：

```c
[SYS_sysinfo] "sysinfo",
```

### 3.6 运行测试

启动 qemu：

```
$ sysinfotest
sysinfotest: OK
```

---

# 四、测试结果与评分

在评分之前可以先在当前lab的仓库的路径下，建立一个 `time.txt` 文本，里面只需要包含你完成这个实验总共耗时（只需要一个数字），例如耗时5小时即写入：5

另外，温馨提醒：不要改动官方 `README` 文档，否则测试出错！

```bash
make grade
```

现在应当能看到 `trace` 与 `sysinfotest` 相关测试通过；其余与本 Lab 无关的会被跳过或已通过。实验页面也展示了 `trace` 的期望输出样例，可对照比对。

![alt text](<Pasted image 20250815174926.png>)

---

# 五、调试与常见坑

- **忘了改 `user/usys.pl`**：用户程序用 `ecall` 需要 stub；没加会编译不过或运行报 “exec 失败”。
    
- **syscall 编号冲突/错序**：确保在 `kernel/syscall.h` 里递增且与 `syscalls[]` 索引一致。
    
- **`fork()` 未继承 `tracemask`**：会导致只父进程追踪、子进程不追踪；专项测试会抓出来。
    
- **`copyout` 忘了/地址错**（`sysinfo`）：一定先用 `argaddr` 取用户指针，再 `copyout`。参考 `sys_fstat` 用法是一种好做法。
    
- **并发访问内核结构未加锁**：统计 `freeram()`、`countproc()` 记得按上面的持锁方式写，避免竞争。
    
- **QEMU/工具版本问题**：按官方工具页检查 QEMU≥5.1 且工具链在 PATH 下。
    
- **GDB 单步排障**：本实验官方附了 gdb 指南与示例断点 `b syscall`，在陷入或崩溃时非常好用。
