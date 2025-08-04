#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <ucontext.h>
#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>

ucontext_t ctx[3];
ucontext_t main_ctx;
// struct ucontext ctx[2]

int count = 0;

// coroutine1
void func1(void) {

	while (count ++ < 30) {
		printf("1\n");
		//swapcontext(&ctx[0], &ctx[1]);
		swapcontext(&ctx[0], &main_ctx);
		printf("4\n");
	}

}
// coroutine2
void func2(void) {

	while (count ++ < 30) {
		printf("2\n");
		//swapcontext(&ctx[1], &ctx[2]);
		swapcontext(&ctx[1], &main_ctx);
		printf("5\n");
	}
}

// coroutine3
void func3(void) {

	while (count ++ < 30) {
		printf("3\n");
		//swapcontext(&ctx[2], &ctx[0]);
		swapcontext(&ctx[2], &main_ctx);
		printf("6\n");
	}
}

int main() {

    char stack1[2048] = {0};
    char stack2[2048] = {0};
    char stack3[2048] = {0};

    getcontext(&ctx[0]); // 设置环境
    ctx[0].uc_stack.ss_sp = stack1;
    ctx[0].uc_stack.ss_size = sizeof(stack1);
    ctx[0].uc_link = &main_ctx; // 下一次执行
    makecontext(&ctx[0], func1, 0);

    getcontext(&ctx[1]);
    ctx[1].uc_stack.ss_sp = stack2;
    ctx[1].uc_stack.ss_size = sizeof(stack2);
    ctx[1].uc_link = &main_ctx; // 下一次执行
    makecontext(&ctx[1], func2, 0);

    getcontext(&ctx[2]);
	ctx[2].uc_stack.ss_sp = stack3;
	ctx[2].uc_stack.ss_size = sizeof(stack3);
	ctx[2].uc_link = &main_ctx;
	makecontext(&ctx[2], func3, 0);

    printf("swapcontext\n");
    //int i = 30;
    while (count <= 30) { // scheduler
		swapcontext(&main_ctx, &ctx[count%3]);
	}

    printf("\n");


    return 0;
}
