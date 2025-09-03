# 实验目标及准备

Lab8 一共分成 **两部分**：

1. **Memory Allocator（中等难度）**：给每个 CPU 一个独立的空闲内存链表，避免单一锁的竞争。把单个全局空闲链表变成每个 CPU 一个链表，空闲时从其它 CPU 窃取一页。降低 kmem.lock 的争用。
    
2. **Buffer Cache（较难）**：把全局 bcache 链表改为哈希桶（每桶一个锁），并用 timestamp 做 LRU 替换，从而把 bcache.lock 的争用降到很低。

**版本控制：**

1. 切到 `lock` 分支并清理、编译基础版本：
	
	```bash
	git fetch
	git checkout lock
	git checkout -b lock_test
	make clean 
	make
	```
	
2. 如果实验失败：
	```bash
	# 放弃工作区所有未暂存的修改（即恢复到最近一次提交的状态） 
	git checkout -- .
	```
	
3. 实验成功合并到主分支

```bash
git checkout lock       # 回到主分支
git merge lock_test    # 合并实验分支的改动
```

- 如果有冲突，Git 会提示手动解决
- 合并后实验改动就永久保存在分支 `pgtbl`
    
4. 删除测试分支：
	- 如果确认 `pgtbl_test` 分支的改动已经成功合并到主分支 `pgtbl` 且不再需要该分支时，可以安全地删除它。
	- 删除本地分支的命令：
	
	```bash
	git checkout lock
	git branch -d lock_test
	```
	
	- 如果需要同时删除远程仓库（比如 GitHub 上）的 `pgtbl_test` 分支，可以执行：
	```bash
	git push github --delete pgtbl_test
	```

---

# Part1：Memory Allocator

xv6 的内存分配器在 `kernel/kalloc.c` 中，原本所有 CPU 共用一个全局空闲链表 `kmem.freelist`，用一把大锁保护。多核环境下会造成严重锁竞争。

> 我们要改成 **每个 CPU 自己维护一份 freelist**，这样不同 CPU 的分配与释放可以并行执行。  
> 如果当前 CPU freelist 空了，就尝试“偷”别的 CPU 的内存。

## 1. 每 CPU 一个 freelist

文件：`kernel/kalloc.c`
**替换原来单个 `kmem` 结构为数组形式**（每个 CPU 一个锁 + freelist），找到原本的定义（大概在文件开头）：

```c
struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;
```

把它改成数组，每个 CPU 一份：

```c
// kernel/kalloc.c 顶部（或原 kmem 定义处）
#include "param.h" // 确保 NCPU 可见
// ...

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];

```

每个 CPU 持有自己的空闲链表和锁，避免单个全局锁的争用。

## 2. 修改 `kinit`

`kinit` 负责初始化 kmem。原来的 `kinit` 里通常是 `initlock(&kmem.lock, "kmem"); freerange(end, (void*)PHYSTOP);`。我们需要对每个 CPU 初始化锁，并让 `freerange` 把所有物理页加入到当前运行 `kinit` 的 CPU（通常是 CPU0）链表。

通常位于`kernel/kalloc.c`，找到 `kinit()` 函数（大概在文件 50 行左右），原本是：

```c
void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}
```

改成初始化每个 CPU 的锁（名字要以 `"kmem"` 开头）：

```c
void
kinit()
{
  char name[16];
  for(int i = 0; i < NCPU; i++){
    snprintf(name, sizeof(name), "kmem_%d", i);
    initlock(&kmem[i].lock, name);
  }
  freerange(end, (void*)PHYSTOP);
}
```

注意：`snprintf` 在 xv6 源中通常可用（查看 `kernel/sprintf.c`），名称要以 `"kmem"` 开头以满足评分要求。


## 3. 修改 `kfree`

用当前 CPU 的 ID 将页推入本地 freelist。**必须在关闭中断时调用 `cpuid()`（即 push_off/pop_off）**，因为 `cpuid()` 的结果只有在中断被禁用时才可靠。

通常位于`kernel/kalloc.c`，在 `kfree(void *pa)` 中，找到原本的代码：

```c
acquire(&kmem.lock);
r->next = kmem.freelist;
kmem.freelist = r;
release(&kmem.lock);
```

修改为：

```c
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  push_off();              // 关闭中断以安全调用 cpuid()
  int id = cpuid();        // 当前 CPU id
  acquire(&kmem[id].lock);
  r->next = kmem[id].freelist;
  kmem[id].freelist = r;
  release(&kmem[id].lock);
  pop_off();               // 恢复中断
}

```

- 使用 `push_off()`/`pop_off()` 包围 `cpuid()` + 操作，防止中断导致迁移 CPU。
- `push_off/pop_off` 关中断，因为 `cpuid()` 只有在关中断时才安全。
- `initlock` 名称已在 `kinit` 中初始化为 `kmem%d`。


## 4. 修改 `freerange`

通常位于同文件 `kernel/kalloc.c`

`freerange` 的代码向 freelist 添加多个页面。不需要大幅改变 `freerange`，但它在把页加入 freelist 时，应把页加入**当前 CPU 的 freelist**（也就是调用 `kinit` 那个 CPU）。原实现通常调用 `kfree(p)` 循环或直接 push 到 `kmem.freelist`。

如果 `freerange` 当前实现是调用 `kfree()`，那我们已经在 `kfree()` 做了 cpu-local 操作（下面会改），所以 `freerange` 可以保持原样。若原实现直接操作 `kmem`, 则需要改为调用 `kfree()`，这样 `kfree` 会将页入到调用者的 CPU freelist（`kinit` 运行在 CPU0，因此页会进入 kmem[0]）。

建议做**最小改动**，确保 `freerange` 调用 `kfree()` 而不是直接改全局列表。示例：

```c
void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}
```



## 5. 修改 `kalloc`

逻辑：先在本 CPU 的 freelist 取；如果没有，从其它 CPU 轮询窃取一页（只窃取一页，避免复杂）。窃取其它 CPU 时要 acquire 目标 CPU 的 lock；为防死锁，获取其它锁时不要在持有本地锁的情况下去 acquire 多个锁（我们下面示例会在本地锁持有期间检查本地 freelist，如果为空则 release 本地锁再去循环 acquire 其他 CPU 的锁——但需要注意并发），我将给一个线程安全且合理的实现。

通常位于`kernel/kalloc.c`，下面是一个清晰实现（替换原来的 `kalloc()`）：

```c
void *
kalloc(void)
{
  struct run *r = 0;

  push_off();
  int id = cpuid();

  // 1) 先尝试从本 CPU freelist 获取
  acquire(&kmem[id].lock);
  if(kmem[id].freelist){
    r = kmem[id].freelist;
    kmem[id].freelist = r->next;
    release(&kmem[id].lock);
    pop_off();
    memset((char*)r, 5, PGSIZE);
    return (void*)r;
  }
  // 本地没有，release 本地 lock，然后遍历其他 cpu 窃取
  release(&kmem[id].lock);

  for(int i = 0; i < NCPU; i++){
    if(i == id) continue;
    acquire(&kmem[i].lock);
    if(kmem[i].freelist){
      r = kmem[i].freelist;
      kmem[i].freelist = r->next;
      release(&kmem[i].lock);
      break;
    }
    release(&kmem[i].lock);
  }
  pop_off();

  if(r)
    memset((char*)r, 5, PGSIZE);
  return (void*)r;
}
```

- 在尝试窃取别的 CPU freelist 时 **不** 持有本地锁（先 release 本地）——避免跨多个锁成环死锁的风险。
    
- 只窃取一页（`r`），这是简单且足够的。
    
- `push_off/pop_off` 包裹全部 `cpuid()` 相关临界区。

## 6. 测试 Memory Allocator

- 确保 `kmem` 名称以 `kmem` 开头（评分要求）。
- `freerange` 必须把页加入到调用者的 freelist（通常调用 `kfree()` 做这事更安全）。
    
- **测试**：
    1. 编译并运行 qemu（多核模拟）：
        
        ```bash
        make qemu CPUS=4
        ```
        
        （建议用 `CPUS=4` 做并行测试）
        
    2. 在 xv6 shell 运行：
        
        ```sh
        kalloctest 
        usertests sbrkmuch 
        usertests
        ```
        
    1. 观察 `kalloctest` 输出，目标：kmem 的 “#fetch-and-add” 应该大大降低或为 0；`test1 OK`。`usertests` 的 `sbrkmuch` 应该通过。
		
		```bash
		init: starting sh
		$ kalloctest 
		start test1
		test1 results:
		--- lock kmem/bcache stats
		lock: bcache: #fetch-and-add 0 #acquire() 1242
		--- top 5 contended locks:
		lock: proc: #fetch-and-add 45848 #acquire() 278058
		lock: proc: #fetch-and-add 37486 #acquire() 278072
		lock: proc: #fetch-and-add 33176 #acquire() 278210
		lock: proc: #fetch-and-add 32271 #acquire() 278097
		lock: proc: #fetch-and-add 31095 #acquire() 278212
		tot= 0
		test1 OK
		start test2
		total free number of pages: 32499 (out of 32768)
		.....
		test2 OK
		$ 
		```
        
- 调试技巧：
    
    - 在 `kalloc`/`kfree` 的关键分支处加入 `cprintf("kalloc: cpu %d stole from %d\n", id, i);`（开发完成后删掉）。
        
    - 如果看到内存泄露或 `panic("kfree")`，检查 `freerange` 是否以页对齐的地址调用 `kfree`。

---

# Part2：Buffer Cache

`kernel/bio.c` 里，所有的 buffer cache 都由一把 `bcache.lock` 保护，导致并发读写文件时大量锁竞争。

我们要改成：

- 用 **哈希表 + 桶锁** 代替全局链表  
- 每个桶管理一部分 buf
- `bget`、`brelse` 等函数只操作对应的桶锁

核心思路：用多个哈希桶（每桶一个自旋锁与链表）来分散锁争用；当找不到目标块时，遍历桶找一个 `refcnt==0` 且 timestamp 最早的块做回收（LRU）。移动 buf 从一个桶到另一个桶要小心锁的顺序避免死锁（按桶 id 的顺序去 acquire）。

## 1. 在 `kernel/buf.h` 中添加 timestamp 字段

找到 `struct buf` 定义（通常在 `kernel/buf.h`）并添加 `uint timestamp;`：

```c
struct buf {
  int valid;   // has data been read from disk?
  int disk;    // does disk "own" buf?
  uint dev;
  uint blockno;
  struct sleeplock lock;
  uint refcnt;
  struct buf *prev; // LRU / bucket list
  struct buf *next;
  uchar data[BSIZE];
  uint timestamp; // 新增：记录最近一次释放/使用的 ticks
};
```

**注意**：`ticks` 全局在 `kernel/trap.c`/`kernel/ticks.c`，更新时间戳时必须获取 `tickslock`。

## 2. 定义哈希桶

在 `kernel/bio.c` 中定义哈希桶结构与宏，在 `kernel/bio.c` 的顶部（替换或添加）：

```c
#define NBUCKET 13      // 素数，经验值，可调整
#define HASH(dev, blockno) (((dev) + (blockno)) % NBUCKET)

struct bucket {
  struct buf head;      // 哨兵节点，循环链表
  struct spinlock lock; // 名称需以 "bcache" 开头
};
```

然后把原先的 `bcache` 结构（如果存在一个全局 `bcache` 有 `lock` 与 `head`）替换为：

```c
struct {
  struct buf buf[NBUF];
  struct bucket buckets[NBUCKET];
} bcache;
```

每个 `bucket` 有一个哨兵 `head`（循环链表），所有 `buf` 最初加入 bucket 0（或均匀分布均可）。每个 `bucket.lock` 初始化时要传名以 `bcache` 开头（评分要求）。


## 3. 修改 `binit`初始化所有桶与缓冲区

在 `binit()` 中，初始化每个 bucket 的 lock 与 head，并把所有 `buf` 放到 bucket 0（或初始分散也可以，但示例放到 bucket 0）：

```c
void
binit(void)
{
  struct buf *b;
  char name[16];

  // 初始化 bucket locks 和头节点（空循环链表）
  for(int i = 0; i < NBUCKET; i++){
    snprintf(name, sizeof(name), "bcache.%d", i); // 名称要以 bcache 开头
    initlock(&bcache.buckets[i].lock, name);
    bcache.buckets[i].head.next = &bcache.buckets[i].head;
    bcache.buckets[i].head.prev = &bcache.buckets[i].head;
  }

  // 初始化 buf 数组并把它们都放入 bucket 0
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    initsleeplock(&b->lock, "buffer");
    // 插入到 bucket 0 的 head 后面
    b->next = bcache.buckets[0].head.next;
    b->prev = &bcache.buckets[0].head;
    bcache.buckets[0].head.next->prev = b;
    bcache.buckets[0].head.next = b;
    b->timestamp = 0;
    b->refcnt = 0;
    b->valid = 0;
    b->dev = 0;
    b->blockno = 0;
  }
}

```

## 4. 修改 `brelse`

只获得对应 bucket 的锁并更新 timestamp / refcnt

原来 `brelse` 可能会用全局 `bcache.lock`。现在实现应：找到 `brelse(struct buf *b)`，原本是：

```c
acquire(&bcache.lock);
b->refcnt--;
release(&bcache.lock);
```

改成：

```c
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int bid = HASH(b->dev, b->blockno);
  acquire(&bcache.buckets[bid].lock);
  if(b->refcnt <= 0)
    panic("brelse: refcnt");
  b->refcnt--;
  if(b->refcnt == 0){
    // 更新时间戳（受 tickslock 保护）
    acquire(&tickslock);
    b->timestamp = ticks;
    release(&tickslock);
  }
  release(&bcache.buckets[bid].lock);
}

```
- `b->dev` 和 `b->blockno` 在释放时应该仍然表示该 buf 所缓存的块；如果缓存内容被回收并重新赋值，timestamp 已在 bget 中被重置。
- `holdingsleep(&b->lock)` 检查是否持有 sleep lock（b->lock）。


## 5. 修改 `bget`

`bget(dev, blockno)` 的职责：

1. 在对应 bucket 查找：如果找到并 `refcnt++`，更新时间戳并返回（在返回前应该 `acquiresleep(&b->lock)`）。这里必须先在 bucket 锁下增加 `refcnt`，然后释放 bucket 锁，再 `acquiresleep(&b->lock)`。（注意：acquiresleep 可能睡眠，不可持有 spinlock 时调用）
    
2. 如果没找到：需要在所有桶中寻找 `refcnt==0` 且 timestamp 最小（LRU） 的 buf 作回收；**遍历桶时要小心锁顺序**以避免死锁。常用策略：按升序桶 id 轮询，且当你从一个桶检查到 candidate 时，**保持该桶的锁**直到你决定回收并把该 buf 移动到目标桶（或在操作过程中释放/获取其他锁但以一致顺序）。下面给出一个实现示例：遍历所有桶寻找 LRU candidate，并在找到后把它从旧 bucket 移到目标 bucket（需要同时修改链表指针），然后给这个 buf 初始化 dev/blockno/valid/refcnt 并返回（在释放 bucket lock 之前不要调用 `acquiresleep`，要先 release bucket lock，再 `acquiresleep`）。
    

**注意**：当 candidate 不在目标 bucket 时，移动它时先持有 candidate 所在 bucket 的 lock（我们已经持有），然后操作并转入目标 bucket（需要 target bucket lock）。为避免死锁，先获取较小 id 的锁，再获取较大 id 的锁；因此在移动时请确保锁的获取顺序按桶 id 的升序。下面的示例实现尽量保证锁顺序。

示例实现（替换 `bget`）：

```c
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int bid = HASH(dev, blockno);
  acquire(&bcache.buckets[bid].lock);

  // 先在当前桶中查找
  for(b = bcache.buckets[bid].head.next; b != &bcache.buckets[bid].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      acquire(&tickslock);
      b->timestamp = ticks;
      release(&tickslock);
      release(&bcache.buckets[bid].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // 没有找到，需要回收一个缓冲区
  struct buf *lru_buf = 0; // 用于记录LRU缓冲区
  int lru_bid = bid;       // LRU缓冲区所在的桶ID

  // 首先在当前桶中寻找LRU缓冲区
  for(b = bcache.buckets[bid].head.next; b != &bcache.buckets[bid].head; b = b->next){
    if(b->refcnt == 0 && (lru_buf == 0 || b->timestamp < lru_buf->timestamp)) {
      lru_buf = b;
      lru_bid = bid;
    }
  }

  // 如果当前桶没有找到，遍历其他桶
  if(lru_buf == 0) {
    for(int i = (bid+1) % NBUCKET; i != bid; i = (i+1) % NBUCKET) {
      acquire(&bcache.buckets[i].lock);
      for(b = bcache.buckets[i].head.next; b != &bcache.buckets[i].head; b = b->next) {
        if(b->refcnt == 0 && (lru_buf == 0 || b->timestamp < lru_buf->timestamp)) {
          lru_buf = b;
          lru_bid = i;
        }
      }
      if(lru_buf) {
        // 找到后，保持该桶的锁，break后继续持有
        break;
      } else {
        release(&bcache.buckets[i].lock);
      }
    }
  }

  if(lru_buf == 0) {
    release(&bcache.buckets[bid].lock);
    panic("bget: no buffers");
  }

  // 如果lru_buf不在当前桶，需要将其移动到当前桶
  if(lru_bid != bid) {
    // 从原桶中移除
    lru_buf->next->prev = lru_buf->prev;
    lru_buf->prev->next = lru_buf->next;
    // 释放原桶的锁
    release(&bcache.buckets[lru_bid].lock);

    // 添加到当前桶
    lru_buf->next = bcache.buckets[bid].head.next;
    lru_buf->prev = &bcache.buckets[bid].head;
    bcache.buckets[bid].head.next->prev = lru_buf;
    bcache.buckets[bid].head.next = lru_buf;
  }

  lru_buf->dev = dev;
  lru_buf->blockno = blockno;
  lru_buf->valid = 0;
  lru_buf->refcnt = 1;
  acquire(&tickslock);
  lru_buf->timestamp = ticks;
  release(&tickslock);
  release(&bcache.buckets[bid].lock);
  acquiresleep(&lru_buf->lock);
  return lru_buf;
}
```


## 6. 修改bpin和bunpin函数

在`kernel/bio.c`中，修改bpin和bunpin函数，只获取对应桶的锁。

```c
void
bpin(struct buf *b)
{
  int bid = HASH(b->dev, b->blockno);
  acquire(&bcache.buckets[bid].lock);
  b->refcnt++;
  release(&bcache.buckets[bid].lock);
}

void
bunpin(struct buf *b)
{
  int bid = HASH(b->dev, b->blockno);
  acquire(&bcache.buckets[bid].lock);
  if(b->refcnt <= 0)
    panic("bunpin");
  b->refcnt--;
  release(&bcache.buckets[bid].lock);
}

```

## 7. 测试 Buffer Cache

1. 运行：
    
    ```bash
    make qemu CPUS=4
    bcachetest
    ```
    
    输出应该类似：
    
    ```bash
    init: starting sh
	$ bcachetest
	start test0
	test0 results:
	--- lock kmem/bcache stats
	--- top 5 contended locks:
	lock: virtio_disk: #fetch-and-add 269097 #acquire() 1248
	lock: proc: #fetch-and-add 111403 #acquire() 105158
	lock: time: #fetch-and-add 65489 #acquire() 64854
	lock: proc: #fetch-and-add 49438 #acquire() 104830
	lock: proc: #fetch-and-add 48132 #acquire() 104775
	tot= 0
	test0: OK
	start test1
	test1 OK
	$ 
    ```
    
    并且 bcache 锁的争用次数接近 0。
    
2. 再运行：
    
    ```bash
    usertests
    ```
    
    确认所有测试通过。
    

# 整体测试

```bash
make grade
```

![alt text](<Pasted image 20250902150808.png>)