# 源文件和目标文件
src = $(wildcard *.c)				
obj = $(patsubst %.c, %.o, $(src)) 

# 目标程序名
target = app

# 编译器和编译选项
CC = gcc
CFLAGS = -Wall -g

# 默认目标
ALL:$(target)

# 生成最终可执行文件
$(target):$(obj)
	$(CC) $(CFLAGS) $^ -o $@

# 编译每个 .c 文件为 .o 文件
$(obj):%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@

# 清理目标
clean:
	-rm -rf $(obj) $(target)

# 声明伪目标
.PHONY:clean ALL