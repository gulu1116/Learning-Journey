#include <stdio.h>
// 声明外部环境变量数组
extern char **environ;

int main(void) {
    int i;
    // 遍历 environ 数组，直到遇到 NULL 结束
    for (i = 0; environ[i] != NULL; i++) {
        // 打印每个环境变量
        printf("%s\n", environ[i]);
    }
    return 0;
}