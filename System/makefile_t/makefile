src = $(wildcard *.c)				
obj = $(patsubst %.c, %.o, $(src)) 
# src = add.c sub.c mul.c hello.c
# obj = add.o sub.o mul.o hello.o

target = hello

# 使用gcc覆盖CC
CC = gcc
ALL:$(target)

$(target):$(obj)
	$(CC) $^ -o $@

$(obj):%.o:%.c
	$(CC) -c $< -o $@

clean:
	-rm -rf $(obj) $(target)

.PHONY:clean ALL