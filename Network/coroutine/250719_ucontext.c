#include <stdio.h>
#include <ucontext.h>

ucontext_t ctx[3];
ucontext_t main_ctx;

int count = 0;

void func1(void) {
    while (count ++ < 30) {
        printf("1\n");
        swapcontext(&ctx[0], &ctx[1]);
        printf("4\n");
    }
}

void func2(void) {
    // char a[4096] = {0};
    while (count ++ < 30) {
        printf("2\n");
        swapcontext(&ctx[1], &ctx[2]);
        printf("5\n");
    }
}

void func3(void) {
    // char a[4096] = {0};
    while (count ++ < 30) {
        printf("3\n");
        swapcontext(&ctx[2], &ctx[0]);
        printf("6\n");
    }
}

int main() {
    char stack1[2048] = {0};
    char stack2[2048] = {0};
    char stack3[2048] = {0};

    getcontext(&ctx[0]);
    ctx[0].uc_stack.ss_sp = stack1;
    ctx[0].uc_stack.ss_size = sizeof(stack1);
    ctx[0].uc_link = &main_ctx;
    makecontext(&ctx[0], func1, 0);

    getcontext(&ctx[1]);
    ctx[1].uc_stack.ss_sp = stack2;
    ctx[1].uc_stack.ss_size = sizeof(stack2);
    ctx[1].uc_link = &main_ctx;
    makecontext(&ctx[1], func2, 0);

    getcontext(&ctx[2]);
    ctx[2].uc_stack.ss_sp = stack3;
    ctx[2].uc_stack.ss_size = sizeof(stack3);
    ctx[2].uc_link = &main_ctx;
    makecontext(&ctx[2], func3, 0);

    printf("swapcontext\n");
    swapcontext(&main_ctx, &ctx[0]);

    printf("\n");
}