# 实验准备


切换到 `net` 分支

```sh
git fetch
git checkout net
```


清理并准备编译

为了避免之前实验的生成文件干扰，先清理：

```sh
make clean
```

编译检查：在正式写代码前，先试一下能不能编译通过：

```sh
make qemu
```

如果能正常进入 xv6 的 shell（提示符 `$` 出现），说明环境没问题。

运行网络测试（初始状态），在一个终端里运行：

```sh
make server
```

它会启动宿主机上的 “server 程序”，模拟外部网络环境。

再在另一个终端里运行：

```sh
make qemu
```

进入 xv6 后执行：

```sh
nettests
```

此时，由于 `e1000_transmit` 和 `e1000_recv` 还没实现，大多数测试会卡住或失败。这是 **预期现象**，

---

# 实验概述

- **TX / RX ring（环）**：网卡和驱动通过一组描述符（descriptor）交换数据。驱动把要发的数据放在 TX 描述符里并推动 `TDT`（Transmit Descriptor Tail）寄存器，网卡读描述符并发送，发送完成后在描述符的 status 上置位 `DD`（Descriptor Done）。接收类似：网卡把收到的数据写入 RX 描述符指定的内存，然后置 RX 描述符的 `DD`，驱动检查并处理然后把 descriptor 返还给网卡（更新 `RDT`）。
    
- **关键寄存器**：
    
    - `E1000_TDT`：TX Tail — 驱动写它告诉设备新的尾索引。
        
    - `E1000_RDT`：RX Tail — 驱动写它告诉设备最新已处理的 RX 描述符索引。
        
- **关键位/标志**：
    
    - `E1000_TXD_CMD_RS`、`E1000_TXD_CMD_EOP`：TX 描述符的 cmd 标志，常见组合是 `RS|EOP`（要求网卡对该描述符产生报告并表明这是包的结尾）。
        
    - `E1000_TXD_STAT_DD`、`E1000_RXD_STAT_DD`：描述符 status 中的 “done” 位（是否完成）。
        
- **mbuf**：xv6 的网络缓冲结构，`m->head` 是数据缓冲区地址，`m->len` 是长度。驱动会保存 mbuf 指针以便在设备发送完（DD 置位）后释放它。
    

**在做代码修改前（建议）**

1. 在 `kernel/e1000.c` 顶部确认你能访问到这些符号：`regs`、`tx_ring`、`rx_ring`、`tx_mbufs`、`rx_mbufs`、`e1000_lock`、`mbufalloc()`、`mbuffree()`、`net_rx()`、`cprintf()` / `panic()`、`E1000_*` 常量、`RX_RING_SIZE`、`TX_RING_SIZE`。这些在实验模板里通常已经定义好。
    
2. 先在 `e1000_transmit()`、`e1000_recv()` 增加 `cprintf("e1000_transmit called\n")` / `cprintf("e1000_recv called\n")` 之类打印，编译、运行 `make server`、在 xv6 里运行 `nettests`，确认函数会被调用并观察打印位置（这一步你已在提示里做过，推荐先确保 I/O 通路通）。
    
3. 记得在做并发访问时使用 `acquire(&e1000_lock)` / `release(&e1000_lock)`（模板通常已有），防止中断/多核并发问题。
    

---

# 实验流程

**修改位置：文件`kernel/e1000.c`**

## 1) e1000_transmit（写入 TX 环）

`e1000_transmit` 的实现：

```c
int
e1000_transmit(struct mbuf *m)
{
  uint32 tdt;
  // 并发安全，保护对环和寄存器的访问
  acquire(&e1000_lock);

  // 1) 读网卡期望的下一个 TX 索引（TDT）
  tdt = regs[E1000_TDT];

  // 2) 检查对应的描述符是否已完成（DD = 1），如果没有，说明环已满
  if (!(tx_ring[tdt].status & E1000_TXD_STAT_DD)) {
    // 无可用描述符：失败，调用者会释放 mbuf
    release(&e1000_lock);
    return -1;
  }

  // 3) 如果该槽之前保存了一个 mbuf，说明之前发送已完成，但尚未释放，释放它
  if (tx_mbufs[tdt]) {
    mbuffree(tx_mbufs[tdt]);
    tx_mbufs[tdt] = 0;
  }

  // 4) 填充描述符：地址、长度、cmd
  //    m->head 指向包数据，m->len 是长度
  tx_ring[tdt].addr = (uint64) m->head;
  tx_ring[tdt].length = m->len;

  // 设置需要的命令位（要求报告完成 RS；包结束 EOP）
  tx_ring[tdt].cmd = E1000_TXD_CMD_RS | E1000_TXD_CMD_EOP;

  // 清除 status（网卡会在完成后写回 DD）
  tx_ring[tdt].status = 0;

  // 保存 mbuf 指针，稍后网卡驱动在 descriptor DD 置位时会释放
  tx_mbufs[tdt] = m;

  // 5) 更新 TDT，通知网卡新的尾索引（取模 TX_RING_SIZE）
  regs[E1000_TDT] = (tdt + 1) % TX_RING_SIZE;

  // 6) 内存屏障（确保对描述符的写在写 TDT 之前完成）
  __sync_synchronize();

  release(&e1000_lock);
  return 0;
}
```

- 如果返回 `0` 表示我们已把 mbuf 成功放入 TX 环，网卡会完成发送并把 `DD` 置位；驱动下一次使用该槽时会释放 mbuf。若返回 `-1` 表示环满，调用方应释放 mbuf（通常调用者会在收到 -1 后立即调用 `mbuffree(m)`）。
    
- `__sync_synchronize()` 是一个轻量的 CPU 内存屏障，确保写到 descriptor 的数据对网卡可见（在大多数实现中写 `regs[E1000_TDT]` 已足够，但加屏障更稳健）。
    
- 一定不要在没有 `DD` 时覆盖 `tx_mbufs[tdt]`，否则会泄漏或重复释放。
    

## 2) e1000_recv（处理 RX 环）

`e1000_recv` 的实现：

```c
static void
e1000_recv(void)
{
  acquire(&e1000_lock);

  // 循环处理所有已到达、还未处理的描述符
  while (1) {
    // 网卡的 RDT 指向最后一个驱动通知给网卡的已空 descriptor。
    // 下一个有数据（如果有）的 descriptor 索引是 RDT + 1 (mod RX_RING_SIZE)
    uint32 rdt = (regs[E1000_RDT] + 1) % RX_RING_SIZE;

    // 检查该描述符是否已由网卡写入完（DD）
    if (!(rx_ring[rdt].status & E1000_RXD_STAT_DD)) {
      // 没有更多接收包
      break;
    }

    // 取出对应的 mbuf，设置长度，由网卡写回的 length 字段给出
    struct mbuf *m = rx_mbufs[rdt];
    if (!m) {
      // 不应该发生：如果没有 mbuf 就 panic
      panic("e1000_recv: missing mbuf");
    }

    // rx descriptor 的 length 字段包含接收到的包长
    m->len = rx_ring[rdt].length;

    // 为该 slot 分配一个新的 mbuf 以供网卡下次接收使用
    struct mbuf *newm = mbufalloc(0);
    if (!newm) {
      // 分配失败：无法继续接收。为了安全起见 panic（也可以 free 原 mbuf 并退出循环）
      panic("e1000_recv: mbufalloc failed");
    }

    // 把新的 mbuf 放回 descriptor，并清除 status
    rx_mbufs[rdt] = newm;
    rx_ring[rdt].addr = (uint64)newm->head;
    rx_ring[rdt].status = 0;

    // 更新 RDT 为刚刚处理的这个索引，告诉网卡这个 descriptor 已经可用
    regs[E1000_RDT] = rdt;

    // 内存屏障，确保写回 RDT 在前面的 descriptor 写完成后对网卡可见
    __sync_synchronize();

    // 先释放锁再把包提交给网络栈（net_rx 可能会产生调度/阻塞）
    release(&e1000_lock);
    net_rx(m);
    // 处理完后重新获取锁，继续循环处理更多包
    acquire(&e1000_lock);
  }

  release(&e1000_lock);
}
```

- `regs[E1000_RDT]` 指的是网卡“已知可用”描述符的尾部索引。用 `RDT + 1` 找到下一个由网卡填写的描述符（若存在）。
    
- 每处理一个已完成的描述符，**必须**为这个槽分配一个新的 mbuf 并把它写回 `addr`；否则网卡下次会把数据写到已经交给内核处理的缓冲区，造成数据紊乱。
    
- 先把新的 mbuf 写回 descriptor，再把 `regs[E1000_RDT] = rdt` 写回，这个顺序很重要（保证网卡不会覆盖我们刚刚交给网络栈处理的数据）。
    
- 处理收到的包时先 `release(&e1000_lock)` 是必要的（`net_rx()` 可能做较慢的工作或阻塞），避免长时间持锁阻塞中断处理或其他 CPU。
    


---

# 测试

**1. 确保代码编译通过**

先清理并重新编译：

```sh
make clean
make
```

**2. 启动模拟的网络环境**

Lab11 提供了一个用户态的 **server 程序**，模拟外部网络（提供 ping/dns/udp 测试）。  
在一个终端窗口里执行：

```sh
make server
```

终端会卡住运行，等待 xv6 的内核与之通信。  
注意：测试过程中server 要**一直保持运行**，不能关掉

**3. 启动 xv6 内核**

在另一个终端窗口里执行：

```sh
make qemu
```

在 xv6 的 shell 提示符 `$` 输入：

```sh
nettests
```


**4. 预期测试输出**

如果 `e1000_transmit` 和 `e1000_recv` 正确实现，会看到类似输出：

```shell
hart 1 starting
hart 2 starting
init: starting sh
$ nettests
nettests running on port 26099
testing ping: OK
testing single-process pings: OK
testing multi-process pings: OK
testing DNS
DNS arecord for pdos.csail.mit.edu. is 128.52.129.126
DNS OK
all tests passed.
$ 
```


**5. 使用 `make grade` 自动打分**

退出 xv6（`Ctrl+A X` 关掉 qemu），然后在宿主机执行：

```sh
make grade
```

它会自动运行一系列测试（包括 `nettests`），并给出结果。  

![alt text](<Pasted image 20250905175504.png>)