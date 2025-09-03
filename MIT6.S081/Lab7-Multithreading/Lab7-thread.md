
# 一、总体说明

这个实验分为三个部分：用户级线程切换（Uthread）、使用线程（Using threads）和屏障（Barrier）。

1. Part 1: Uthread: switching between threads (用户级线程切换)
	
	- 在这个部分，我们需要实现一个用户级线程包，具体是完成线程的创建和切换。提供的代码已经给出了框架，我们需要完成上下文切换和线程调度。
2. Part 2: Using threads (使用线程)
	
	- 这个部分中，我们需要修改一个使用哈希表的程序，使其在多线程环境下正确运行。由于多个线程同时操作哈希表可能会导致数据丢失，我们需要通过加锁来保护哈希表。
	  
3. Part 3: Barrier（屏障）
	
	- 在这个部分，我们需要实现一个屏障：即多个线程必须等待所有线程都到达屏障点后才能继续执行。

---

# 二、Part 1：Uthread - 用户级线程切换

## 1) 在 `user/uthread.c` 中定义保存上下文的 `struct tcontext`

**修改位置**：`user/uthread.c` 文件顶部（在 `#includes` 下或现有类型定义区域）

**插入代码**：

```c
// 用户线程的上下文结构体（只保存 callee-saved 寄存器 + ra, sp）
struct tcontext {
  uint64 ra;
  uint64 sp;

  // callee-saved (s0..s11)
  uint64 s0;
  uint64 s1;
  uint64 s2;
  uint64 s3;
  uint64 s4;
  uint64 s5;
  uint64 s6;
  uint64 s7;
  uint64 s8;
  uint64 s9;
  uint64 s10;
  uint64 s11;
};
```

- 我们只保存 **callee-saved** 寄存器（s0..s11）以及 `ra`/`sp`。这是因为 `thread_switch` 作为函数调用时，caller-save 寄存器由调用者自动保存/恢复（ABI），我们只需保存那些 ABI 要求被调用方保存的寄存器以及 `ra`（返回地址）与栈指针 `sp`。这样既正确又节省空间/时间。
    

## 2) 修改线程结构体

**修改位置**：`struct thread` 定义处

**修改代码**：

```c
#define STACK_SIZE 4096

struct thread {
  char            stack[STACK_SIZE];  /* the thread's stack */
  int             state;              /* FREE, RUNNING, RUNNABLE */
  struct tcontext context;            /* 用户线程上下文 */
};
```

- 每个线程保留它自己的 `context`，用于保存/恢复寄存器，以及 `stack` 作为线程栈空间（栈向下增长）。`STACK_SIZE` 是示例数值（通常实验里已有定义）。注意：`context.sp` 会被设置为 `&t->stack + STACK_SIZE`（栈顶地址）。


## 3) 实现上下文切换汇编代码

**修改位置**：`user/uthread_switch.S`（创建或编辑此文件；makefile 通常会把它编译为 uthread 可执行的一部分）

**完整文件内容（替换/创建）**：

```asm
    .text
    .globl thread_switch
/*
 * save the old thread's registers into memory at a0 (old context)
 * restore the new thread's registers from memory at a1 (new context)
 *
 * calling convention:
 *   a0 -> pointer to old context (struct tcontext *)
 *   a1 -> pointer to new context (struct tcontext *)
 *
 * We save ra, sp, s0..s11 from the current context into *a0,
 * then load ra, sp, s0..s11 from *a1, and return (ret) to new ra.
 */

thread_switch:
    /* save callee-saved regs + ra, sp into old context (at a0) */
    sd ra, 0(a0)
    sd sp, 8(a0)
    sd s0, 16(a0)
    sd s1, 24(a0)
    sd s2, 32(a0)
    sd s3, 40(a0)
    sd s4, 48(a0)
    sd s5, 56(a0)
    sd s6, 64(a0)
    sd s7, 72(a0)
    sd s8, 80(a0)
    sd s9, 88(a0)
    sd s10, 96(a0)
    sd s11, 104(a0)

    /* restore registers from new context (at a1) */
    ld ra, 0(a1)
    ld sp, 8(a1)
    ld s0, 16(a1)
    ld s1, 24(a1)
    ld s2, 32(a1)
    ld s3, 40(a1)
    ld s4, 48(a1)
    ld s5, 56(a1)
    ld s6, 64(a1)
    ld s7, 72(a1)
    ld s8, 80(a1)
    ld s9, 88(a1)
    ld s10, 96(a1)
    ld s11, 104(a1)

    ret
```

- `a0`/`a1` 是 RISC-V 的第一个和第二个整型参数寄存器（ABI）。把 `old` 上下文地址放 `a0`，`new` 上下文地址放 `a1`，`thread_switch` 保存当前寄存器到 `*a0`，从 `*a1` 恢复，然后 `ret`（使用恢复后的 `ra`）跳转到新线程的执行位置。

## 4) 修改线程调度器

**修改位置**：uthread 的调度逻辑处，通常在 `thread_scheduler()` 函数里。找出类似以下伪代码并修改：

找到原来“switch threads?” 判断部分并替换为：

```c
/* example variables assumed: struct thread *current_thread, *next_thread, *t; */
if (current_thread != next_thread) {         /* switch threads?  */
  struct thread *old = current_thread;
  struct thread *new = next_thread;

  current_thread = new; /* set global to new before switching */
  /* call assembly switch: save old into old->context, restore new from new->context */
  thread_switch((uint64)&old->context, (uint64)&new->context);
} else {
  next_thread = 0;
}
```

- 某些参考实现将参数顺序反过来（old,new vs new,old），但上面 `thread_switch` 的实现按 `a0=old, a1=new` 保存/恢复。确保你传入的是 `&old->context` 作为第一个参数，`&new->context` 作为第二个参数。若你看到实验提示或参考代码相反，请依据你 `thread_switch` 的实现调整参数顺序一致。官方 lab 示例中也使用 `(uint64)&t->context, (uint64)&current_thread->context)` 形式；关键是传入“被保存的旧 context 地址”和“要恢复的新 context 地址”。
    

- 在执行 `thread_switch` 前，务必先将 `current_thread` 指向 `new`（或保证切换后全局变量反映当前线程）。切换过程是原子的：`thread_switch` 完成后 CPU 处于 `new` 线程的寄存器上下文，返回 `ret` 至 `new` 的 `ra` 指向的位置（通常是 `func` 的地址，详见下一步 `thread_create`）。
    

## 5) 初始化线程上下文

**修改位置**：`uthread` 的 `thread_create()`函数中，为新分配的 `struct thread *t` 设置初始 context。

**修改代码**：

```c
/* Example inside thread_create after allocating a free thread t */
t->context.ra = (uint64)func;                    // set return address to thread function
t->context.sp = (uint64)t->stack + STACK_SIZE;   // set stack pointer to top of thread's stack
/* You may zero other saved regs optionally */
t->state = RUNNABLE;
```

**关键提醒（重要）**：

- **栈生长方向**：RISC-V 与大多数平台一样栈向下生长，所以栈顶是 `&stack + STACK_SIZE`（即 `t->stack + STACK_SIZE`）。不要把 `sp` 设为 `t->stack`（那会把 sp 放在栈底，导致越界）。
    

**解析**：

- 将 `ra` 设为 `func`：当 `thread_switch` 用 `ld ra, ...` 恢复 `ra`，随后 `ret` 指令会跳转到 `func`，从而开始执行该线程的工作。
    
- 将 `sp` 设为线程栈顶，让线程在运行时从其私人栈上分配局部变量等。
    

## 6) 其它注意点

- **保存/恢复寄存器集合必须与 ABI/调用方式一致**：我们仅保存 callee-saved 寄存器和 `ra`/`sp`，这样 `thread_switch` 安全地作为被调用函数使用。不要在 `thread_switch` 中修改 caller-saved（例如 `a0-a7`, `t0-t6`）的值或假定它们被保留。
    
- **避免在内核/用户混淆**：这整个 uthread 是**用户级**线程包，运行在用户进程内部；不涉及内核 trapframe 或页表切换。实现简单但需小心栈和 ra 的设定。
    
- **Makefile**：确认 `user/uthread_switch.S` 被编译——官方 Makefile 已包含规则（lab 自带）。如果没有，确保 `uthread` 的 target 把 `.S` 文件加入。通常不需要改。

## 7）测试

`make qemu` 编译，执行 `uthread` 查看结果

```shell
xv6 kernel is booting

hart 1 starting
hart 2 starting
init: starting sh
$ uthread 
thread_a started
thread_b started
thread_c started
thread_c 0
thread_a 0
thread_b 0
thread_c 1
...
thread_c 97
thread_a 97
thread_b 97
thread_c 98
thread_a 98
thread_b 98
thread_c 99
thread_a 99
thread_b 99
thread_c: exit after 100
thread_a: exit after 100
thread_b: exit after 100
thread_schedule: no runnable threads
$ 
```


---

# 三、Part2：Using threads - 使用线程

## 1) 对每个散列表（bucket）加锁

在 `notxv6/ph.c` 文件中，添加锁的定义和初始化

**说明**：表大小 `NBUCKET`（实验例里是 5）。为每个 bucket 建一把互斥锁。

**示例代码**（在文件顶部或初始化区域）：

```c
// 在文件开头添加
pthread_mutex_t lock[NBUCKET];  // 每个散列桶一个锁

// 在main函数中添加锁的初始化
int
main(int argc, char *argv[])
{
  // ... 其他代码
  
  // 初始化锁
  for (int i = 0; i < NBUCKET; i++) {
    pthread_mutex_init(&lock[i], NULL);
  }
  
  // ... 其他代码
}
```

## 2) 在put函数中对插入操作加锁

**位置**：在put函数中对插入操作加锁

```c
static
void put(int key, int value)
{
  int i = key % NBUCKET;
  
  // 检查键是否已存在
  struct entry *e = 0;
  for (e = table[i]; e != 0; e = e->next) {
    if (e->key == key)
      break;
  }
  
  if(e){
    // 更新现有键的值
    e->value = value;
  } else {
    // 新键，需要插入
    pthread_mutex_lock(&lock[i]);  // 加锁
    insert(key, value, &table[i], table[i]);
    pthread_mutex_unlock(&lock[i]);  // 解锁
  }
}
```

- 插入是“头插法”，两线程同时插入同一 bucket 会导致链表头竞争与数据丢失。对该 bucket 加锁就能保证串行化修改。不要把单把大锁用于所有 buckets，会损失并行性。

## 3) 编译运行

`make ph` 编译单个 ph 文件，再执行 `./ph 4` 查看结果

```shell
lqf@ubuntu:~/share1/6.s081-xv6-labs$ make ph
gcc -o ph -g -O2 notxv6/ph.c -pthread
lqf@ubuntu:~/share1/6.s081-xv6-labs$ ./ph 4
100000 puts, 2.596 seconds, 38515 puts/second
2: 0 keys missing
3: 0 keys missing
0: 0 keys missing
1: 0 keys missing
400000 gets, 10.020 seconds, 39920 gets/second
```

---

# 四、Part3：Barrier - 屏障

在 `notxv6/barrier.c` 文件中，实现 `barrier` 函数：

```c
static void 
barrier()
{
  // 申请持有锁
  pthread_mutex_lock(&bstate.barrier_mutex);
  
  bstate.nthread++;
  if(bstate.nthread == nthread) {
    // 所有线程已到达
    bstate.round++;
    bstate.nthread = 0;
    pthread_cond_broadcast(&bstate.barrier_cond);
  } else {
    // 等待其他线程
    pthread_cond_wait(&bstate.barrier_cond, &bstate.barrier_mutex);
  }
  
  // 释放锁
  pthread_mutex_unlock(&bstate.barrier_mutex);
}
```

- 经典 barrier：每个线程到达 `barrier()` 增加计数；最后一个到达的线程广播唤醒其他线程并把计数归零。`pthread_cond_wait` 在内部会 atomically 解锁 mutex 并阻塞，然后被唤醒时重新获得锁继续。上述实现与实验要求一致。
    

**编译和测试**

完成所有代码修改后，按照以下步骤编译和测试：

1. 编译并运行uthread测试：
```
make qemu
$uthread
```

2. 编译并运行ph测试（使用线程）：
```
make ph
./ph 1  // 单线程测试
./ph 2  // 多线程测试
```

```shell
lqf@ubuntu:~/share1/6.s081-xv6-labs$ ./ph 1 
100000 puts, 11.183 seconds, 8942 puts/second
0: 0 keys missing
100000 gets, 11.240 seconds, 8897 gets/second
lqf@ubuntu:~/share1/6.s081-xv6-labs$ ./ph 2 
100000 puts, 5.259 seconds, 19016 puts/second
0: 0 keys missing
1: 0 keys missing
200000 gets, 10.763 seconds, 18582 gets/second
```

3. 编译并运行barrier测试：
```
make barrier
./barrier 2  // 使用2个线程测试
```

---


# 五、测试成绩

`make grade`

![alt text](<Pasted image 20250901014144.png>)