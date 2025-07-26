# 源文件列表
src = $(wildcard *.c)

# 目标程序：每个源文件去掉 .c 后缀
target = $(basename $(src))

# 编译器和编译选项
CC = gcc
CFLAGS = -Wall -g

# 默认目标：编译所有可执行文件
ALL: $(target)

# 每个 .c 文件编译成无后缀的可执行文件
%: %.c
	$(CC) $(CFLAGS) $< -o $@

# 清理所有生成的文件
clean:
	-rm -f $(target)

# 声明伪目标
.PHONY: ALL clean
