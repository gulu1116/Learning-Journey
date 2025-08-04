#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <ucontext.h>
#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <poll.h>
#include <stdlib.h>

#define MAX_COROUTINES 10
#define STACK_SIZE (16 * 1024)

typedef struct {
    ucontext_t ctx;
    char stack[STACK_SIZE];
    int id;
    int fd;
    int is_used;
} coroutine_t;

coroutine_t coroutines[MAX_COROUTINES];
ucontext_t main_ctx;
int epfd;
int current_coro = -1;

// Hook declarations
typedef ssize_t (*read_t)(int fd, void *buf, size_t count);
typedef ssize_t (*write_t)(int fd, const void *buf, size_t count);
typedef int (*poll_t)(struct pollfd *fds, nfds_t nfds, int timeout);

read_t real_read = NULL;
write_t real_write = NULL;
poll_t real_poll = NULL;

// 协程调度器
void scheduler();

// 协程入口函数
void coro_entry();

// 初始化协程系统
void init_coroutines() {
    epfd = epoll_create1(0);
    if (epfd < 0) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }
    
    memset(coroutines, 0, sizeof(coroutines));
    for (int i = 0; i < MAX_COROUTINES; i++) {
        coroutines[i].id = i;
        coroutines[i].is_used = 0;
    }
}

// 创建新协程
int create_coroutine() {
    for (int i = 0; i < MAX_COROUTINES; i++) {
        if (!coroutines[i].is_used) {
            coroutines[i].is_used = 1;
            getcontext(&coroutines[i].ctx);
            coroutines[i].ctx.uc_stack.ss_sp = coroutines[i].stack;
            coroutines[i].ctx.uc_stack.ss_size = STACK_SIZE;
            coroutines[i].ctx.uc_link = &main_ctx;
            
            makecontext(&coroutines[i].ctx, coro_entry, 0);
            return i;
        }
    }
    return -1;
}

// 协程入口函数
void coro_entry() {
    // 这里是协程的业务逻辑
    printf("Coroutine %d started\n", current_coro);
    
    // 示例I/O操作
    int fd = open("a.txt", O_CREAT | O_RDWR, 0644);
    if (fd < 0) {
        perror("open");
        return;
    }
    
    char *str = "1234567890";
    write(fd, str, strlen(str));
    lseek(fd, 0, SEEK_SET);
    
    char buffer[128] = {0};
    read(fd, buffer, sizeof(buffer));
    
    printf("Coroutine %d read: %s\n", current_coro, buffer);
    close(fd);
    
    // 协程结束
    coroutines[current_coro].is_used = 0;
    current_coro = -1;
}

// Hooked poll
int poll(struct pollfd *fds, nfds_t nfds, int timeout) {
    if (!real_poll) {
        real_poll = dlsym(RTLD_NEXT, "poll");
    }
    
    int ready = real_poll(fds, nfds, 0);
    if (ready > 0) {
        return ready;
    }
    
    // 添加fd到epoll
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = fds[0].fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fds[0].fd, &ev);
    
    // 保存协程状态
    coroutines[current_coro].fd = fds[0].fd;
    
    // 切换到调度器
    swapcontext(&coroutines[current_coro].ctx, &main_ctx);
    
    // 当协程被重新调度时，确保fd已就绪
    return 1;
}

// Hooked read
ssize_t read(int fd, void *buf, size_t count) {
    if (!real_read) {
        real_read = dlsym(RTLD_NEXT, "read");
    }
    
    struct pollfd pfd = {fd, POLLIN, 0};
    if (poll(&pfd, 1, 0) > 0) {
        return real_read(fd, buf, count);
    }
    
    // 不会执行到这里，因为poll会触发协程切换
    return -1;
}

// Hooked write
ssize_t write(int fd, const void *buf, size_t count) {
    if (!real_write) {
        real_write = dlsym(RTLD_NEXT, "write");
    }
    
    struct pollfd pfd = {fd, POLLOUT, 0};
    if (poll(&pfd, 1, 0) > 0) {
        return real_write(fd, buf, count);
    }
    
    // 不会执行到这里
    return -1;
}

// 协程调度器
void scheduler() {
    struct epoll_event events[MAX_COROUTINES];
    
    while (1) {
        int active_coros = 0;
        for (int i = 0; i < MAX_COROUTINES; i++) {
            if (coroutines[i].is_used) {
                active_coros = 1;
                break;
            }
        }
        
        if (!active_coros) break;
        
        int n = epoll_wait(epfd, events, MAX_COROUTINES, -1);
        for (int i = 0; i < n; i++) {
            int ready_fd = events[i].data.fd;
            
            // 找到等待该fd的协程
            for (int j = 0; j < MAX_COROUTINES; j++) {
                if (coroutines[j].is_used && coroutines[j].fd == ready_fd) {
                    epoll_ctl(epfd, EPOLL_CTL_DEL, ready_fd, NULL);
                    current_coro = j;
                    swapcontext(&main_ctx, &coroutines[j].ctx);
                }
            }
        }
    }
}

int main() {
    // 初始化协程系统
    init_coroutines();
    
    // 创建多个协程
    for (int i = 0; i < 3; i++) {
        int coro_id = create_coroutine();
        if (coro_id >= 0) {
            current_coro = coro_id;
            swapcontext(&main_ctx, &coroutines[coro_id].ctx);
        }
    }
    
    // 启动调度器
    scheduler();
    
    close(epfd);
    return 0;
}