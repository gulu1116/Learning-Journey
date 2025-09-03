# 实验目标与准备

目标：

- **Large files**：在 `fs.h` 增加二级（double）间接块支持，使文件最大可寻址块数变成 `NDIRECT + NINDIRECT + NDINDIRECT`。将一个 direct 改为二级间接（on-disk inode 的 `addrs` 总数保持不变，仍为 13）。
    
- **Symbolic links**：实现 `symlink(target, linkpath)` 系统调用、新增 `T_SYMLINK`、新增 `O_NOFOLLOW`，并让 `open()` 在未设置 `O_NOFOLLOW` 时自动跟随符号链接（有限深度）。
    

修改完成后记得：

```sh
make clean
make
# 运行 xv6 / qemu 进行测试
make qemu
```

（修改 `NDIRECT` / inode 布局后必须 `make clean`，否则旧的 fs.img 与内核不一致会出问题。）

---

# Part1：Large files（二级间接）

## 1) 在 `kernel/fs.h` 中添加/修改宏

**文件**：`kernel/fs.h`（或项目中对应的 fs 头文件）  
**在宏定义区替换 / 添加（准确文本）**：

```c
// --- 在 fs.h 顶部的宏区域 ---
// 将 NDIRECT 改为 11，并定义 NINDIRECT、NDINDIRECT、MAXFILE、NADDR_PER_BLOCK
#define NDIRECT 11
#define NINDIRECT (BSIZE / sizeof(uint))
#define NDINDIRECT ((BSIZE / sizeof(uint)) * (BSIZE / sizeof(uint)))
#define MAXFILE (NDIRECT + NINDIRECT + NDINDIRECT)
#define NADDR_PER_BLOCK (BSIZE / sizeof(uint))  // 一个块中的地址数量
```

**说明**：

- `NDIRECT` 从 12 变为 11，把原来的第 12 个 direct 改为 single indirect，第 13 个改为 double indirect（因此 on-disk `addrs` 总数仍需为 13）。
    
- `MAXFILE` 相应扩大到支持 double-indirect。
    

## 2) 修改 on-disk `dinode` 与内存 `inode` 的 `addrs` 长度

**文件**：`kernel/fs.h`（`struct dinode`）与 `kernel/file.h` 或 `kernel/fs.h`（内存 `struct inode`）  
**替换为**：

```c
// fs.h 中 dinode
struct dinode {
  short type;
  short major;
  short minor;
  short nlink;
  uint size;
  uint addrs[NDIRECT + 2];   // 11 direct, 1 single-indirect, 1 double-indirect
};
```

```c
// file.h 或 fs.h 中内存 inode
struct inode {
  // ...
  uint addrs[NDIRECT + 2];
  // ...
};
```

**说明**：`NDIRECT+2` == 13，保持 on-disk inode `addrs` 数量与旧版一致，避免磁盘映像结构不匹配。

## 3) 修改 `bmap()` 支持二级索引

**文件**：`kernel/fs.c`（或 `fs.c` 在你的源码中）  
**找到并替换 `bmap()` 为如下实现**：

```c
static uint
bmap(struct inode *ip, uint bn)
{
  uint addr, *a;
  struct buf *bp;

  // 1) direct blocks
  if(bn < NDIRECT){
    if((addr = ip->addrs[bn]) == 0){
      addr = balloc(ip->dev);
      ip->addrs[bn] = addr;
    }
    return addr;
  }
  bn -= NDIRECT;

  // 2) single indirect
  if(bn < NINDIRECT){
    // ip->addrs[NDIRECT] is single-indirect block
    if((addr = ip->addrs[NDIRECT]) == 0){
      ip->addrs[NDIRECT] = addr = balloc(ip->dev);
    }
    bp = bread(ip->dev, ip->addrs[NDIRECT]);
    a = (uint*)bp->data;
    if((addr = a[bn]) == 0){
      a[bn] = addr = balloc(ip->dev);
      log_write(bp);   // 写回间接块
    }
    brelse(bp);
    return addr;
  }
  bn -= NINDIRECT;

  // 3) double indirect
  if(bn < NDINDIRECT) {
    int level2_idx = bn / NADDR_PER_BLOCK;  // double-index block 中的位置（指向某个 single-indirect block）
    int level1_idx = bn % NADDR_PER_BLOCK;  // 在该 single-indirect block 中的位置
    // ip->addrs[NDIRECT + 1] is the double-indirect block
    if((addr = ip->addrs[NDIRECT + 1]) == 0)
      ip->addrs[NDIRECT + 1] = addr = balloc(ip->dev);

    // 读出 double-indirect block
    bp = bread(ip->dev, addr);
    a = (uint*)bp->data;

    // a[level2_idx] 是指向某个 single-indirect 块的地址
    if((addr = a[level2_idx]) == 0) {
      a[level2_idx] = addr = balloc(ip->dev);    // 分配 single-indirect block
      log_write(bp);    // 修改 double-indirect block，要写回
    }
    brelse(bp);

    // 读出那块 single-indirect block
    bp = bread(ip->dev, addr);
    a = (uint*)bp->data;
    if((addr = a[level1_idx]) == 0) {
      a[level1_idx] = addr = balloc(ip->dev);   // 分配数据块
      log_write(bp);    // 写回 single-indirect 块
    }
    brelse(bp);
    return addr;
  }

  panic("bmap: out of range");
  return 0;
}
```

**说明要点**：

- 当请求的逻辑块号 `bn` 在 direct 范围内，直接使用 `ip->addrs[bn]`。
    
- 如果落在 single indirect 范围，使用 `ip->addrs[NDIRECT]` 的块作为间接表。
    
- 如果落在 double indirect 范围（`bn < NDINDIRECT`），先通过 `ip->addrs[NDIRECT+1]`（double block）找到对应的 single-indirect 块地址，再在该 single-indirect 块中找到/分配真正的数据块。
    
- 每次修改间接表或 double-indirect block 时用 `log_write(bp)` 写回（与 lab 的日志 API 一致）。
    

## 4) 修改 `itrunc()` 释放所有块（包括 double-indirect）

**文件**：`kernel/fs.c`，替换 `itrunc()` 或在合适位置合并下面代码：

```c
void
itrunc(struct inode *ip)
{
  int i, j;
  struct buf *bp;
  uint *a;

  // 释放 direct 块
  for(i = 0; i < NDIRECT; i++){
    if(ip->addrs[i]){
      bfree(ip->dev, ip->addrs[i]);
      ip->addrs[i] = 0;
    }
  }

  // 释放 single indirect 块（ip->addrs[NDIRECT]）
  if(ip->addrs[NDIRECT]){
    bp = bread(ip->dev, ip->addrs[NDIRECT]);
    a = (uint*)bp->data;
    for(i = 0; i < NADDR_PER_BLOCK; i++){
      if(a[i])
        bfree(ip->dev, a[i]);
    }
    brelse(bp);
    bfree(ip->dev, ip->addrs[NDIRECT]);
    ip->addrs[NDIRECT] = 0;
  }

  // 释放 double indirect 的全部内容（ip->addrs[NDIRECT+1]）
  struct buf* bp1;
  uint* a1;
  if(ip->addrs[NDIRECT + 1]) {
    bp = bread(ip->dev, ip->addrs[NDIRECT + 1]);
    a = (uint*)bp->data;
    for(i = 0; i < NADDR_PER_BLOCK; i++) {
      if(a[i]) {
        // a[i] 指向某个 single-indirect 块
        bp1 = bread(ip->dev, a[i]);
        a1 = (uint*)bp1->data;
        for(j = 0; j < NADDR_PER_BLOCK; j++) {
          if(a1[j])
            bfree(ip->dev, a1[j]);  // 释放 data 块
        }
        brelse(bp1);
        bfree(ip->dev, a[i]);       // 释放该 single-indirect 块
      }
    }
    brelse(bp);
    bfree(ip->dev, ip->addrs[NDIRECT + 1]); // 释放 double-indirect 块本身
    ip->addrs[NDIRECT + 1] = 0;
  }

  ip->size = 0;
  iupdate(ip);
}
```

**说明**：

- 顺序：先释放最内层数据块（data），再释放 single-indirect 块，最后释放 double-indirect 块本身。否则会造成块泄漏。
    

## 5) 编译 & 测试 Part1

1. `make clean && make`（非常重要）
    
2. `make qemu`（或 `make qemu-nox`）进入 xv6 shell。
    
3. 运行课程提供的 `big` 测试程序（如果仓库自带）：
    
    ```sh
    $ bigfile
    ```
    
    `bigfile` 会尝试写大量数据到一个文件以检测大文件支持。
    
4. 测试：
    
    ```sh
    $ dd if=/dev/zero of=bigfile bs=1024 count=50000   # 试写大文件 - 注意空间
    $ ls -l bigfile
    $ rm bigfile
    ```
    
    成功写到非常大的块数且不 panic 表示 bmap/itrunc 工作正常。若出现 panic，检查 `bmap` 的索引计算是否正确或是否漏写 `log_write(bp)`。

    ![alt text](<Pasted image 20250903005939.png>)


---

# Part2：Symbolic links（软链接）

## 1) 配置系统调用（添加到用户端与内核 syscall 表）

**a. `user/usys.pl`**：在 syscall 列表中添加一行（与其它 `SYSCALL(...)` 同一组）：

```perl
entry("symlink");
```

重新 `make` 后会更新 `user/usys.S` / `user/usys.c` 等。

**b. `user/user.h`**：添加用户态原型（方便编译用户程序）：

```c
int symlink(char *target, char *linkpath);
```

**c. `kernel/syscall.h` / `kernel/syscall.c`（或相应的 syscall 表）**：  
如果你的内核手动列出 syscall 表（不是由 usys.pl 自动生成），需要：

- 在 `kernel/syscall.h` 增加 `#define SYS_symlink <num>`。
    
- 在 `kernel/syscall.c` 的 `syscalls[]` 数组加入 `sys_symlink` 的映射。和 ``
	
    ```c
    extern uint64 sys_symlink(void);
    ...
    
    static uint64 (*syscalls[])(void) = {
    ...
    [SYS_close]   sys_close,
	[SYS_symlink] sys_symlink,
	};
    ```

> 注：多数 xv6-labs 项目通过 `user/usys.pl` + `syscall.h` 自动生成，按你仓库现有风格添加即可。

## 2) 在头文件中添加 `O_NOFOLLOW` 与 `T_SYMLINK`

**文件**：`kernel/fcntl.h` 与 `kernel/stat.h`  
**添加**：

```c
// fcntl.h
#define O_NOFOLLOW 0x004   // 你提供的值
```

```c
// stat.h
#define T_SYMLINK 4
```

**说明**：`O_NOFOLLOW` 用于 open 的标志，若设置就不跟随 symlink；`T_SYMLINK` 表示 inode 类型为符号链接。

## 3) 在 `kernel/sysfile.c` 实现 `sys_symlink`

**位置**：`kernel/sysfile.c`（与其它 sys_* 在一起）  
**添加函数**（完整实现，保持你提供的风格）：

```c
static struct inode* create(char *path, short type, short major, short minor);

uint64
sys_symlink(void) {
  char target[MAXPATH], path[MAXPATH];
  struct inode* ip_path;

  // 从用户空间获取两个字符串
  if(argstr(0, target, MAXPATH) < 0 || argstr(1, path, MAXPATH) < 0) {
    return -1;
  }

  begin_op();
  // create 在内核中创建 inode 并返回已加锁的 inode
  ip_path = create(path, T_SYMLINK, 0, 0);
  if(ip_path == 0) {
    end_op();
    return -1;
  }

  // 向 inode 数据块写入 target 路径字符串
  // 注意：writei 的参数签名依据你的内核实现，这里按你提供的风格写：
  if(writei(ip_path, 0, (uint64)target, 0, MAXPATH) < MAXPATH) {
    iunlockput(ip_path);
    end_op();
    return -1;
  }

  iunlockput(ip_path);
  end_op();
  return 0;
}
```

**说明/注意**：

- `create()` 返回**已经加锁**的 `inode*`（与 `sys_open` / `sys_link` 的用法一致），使用完成后必须 `iunlockput(ip)`。
    
- `begin_op()/end_op()` 用于日志事务保护（与 `log_write` 协同）。
    
- `writei` 的参数顺序/类型与仓库的 `writei` 定义保持一致；上面用的是给出的签名风格（`writei(ip, 0, (uint64)target, 0, MAXPATH)`），如果内核 `writei` 签名不同，请按内核定义调整参数顺序 / cast。
    

## 4) 修改 `sys_open()` 支持跟随符号链接

**文件**：`kernel/sysfile.c`（`sys_open` 实现中）  
在 `namei()` / `ilock()` 后加入如下符号链接处理逻辑（替换或插入到 `sys_open()` 的路径解析后、进入 `filealloc()` 前）：

```c
 uint64
sys_open(void)
{
  ...

  if(ip->type == T_DEVICE && (ip->major < 0 || ip->major >= NDEV)){
    ...
  }

  // 处理符号链接
  if(ip->type == T_SYMLINK && !(omode & O_NOFOLLOW)) {
    // 若符号链接指向的仍然是符号链接，则递归的跟随它
    // 直到找到真正指向的文件
    // 但深度不能超过MAX_SYMLINK_DEPTH
    for(int i = 0; i < MAX_SYMLINK_DEPTH; ++i) {
      // 读出符号链接指向的路径
      if(readi(ip, 0, (uint64)path, 0, MAXPATH) != MAXPATH) {
        iunlockput(ip);
        end_op();
        return -1;
      }
      iunlockput(ip);
      ip = namei(path);
      if(ip == 0) {
        end_op();
        return -1;
      }
      ilock(ip);
      if(ip->type != T_SYMLINK)
        break;
    }
    // 超过最大允许深度后仍然为符号链接，则返回错误
    if(ip->type == T_SYMLINK) {
      iunlockput(ip);
      end_op();
      return -1;
    }
  }

  if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
    ...
  }

  ...
  return fd;
}

```

**同时需要**：在 `sys_open` 顶部或文件全局定义宏：

```c
#define MAX_SYMLINK_DEPTH 10
```

（或放在合适的头文件）

**要点**：

- 如果打开的是 `T_SYMLINK` 且 `omode` 没有 `O_NOFOLLOW`，那么读取 symlink 内容（`readi`）得到的字符串作为新的路径进行 `namei()` 解析。
    
- 循环最多 `MAX_SYMLINK_DEPTH` 次以避免循环跟随。
    
- 每次替换都会 `iunlockput` 原来的 inode 并 `ilock` 新的 `ip`，因此后面对 `ip` 的处理（创建 `file`、`fd` 等）不需额外改动。
    
- 如果 `O_NOFOLLOW` 被设置，则 `open` 返回 symlink 本身（例如可以 `read` symlink 内容作为普通文件）。
    

## 5) 将 `symlink` 用户程序加入镜像

如果你要在 xv6 镜像中直接运行测试程序，把 `user/symlinktest.c`（如果有）加入 Makefile 的 `UPROGS`。

## 6) 测试 Part2（符号链接）

在 xv6 shell 中测试：

```sh
$ symlinktest
Start: test symlinks
test symlinks: ok
Start: test concurrent symlinks
test concurrent symlinks: ok
$ usertests
...
ALL TESTS PASSED
$
```

---

# 整体测试

```shell
make grade
```

![alt text](<Pasted image 20250903114835.png>)
