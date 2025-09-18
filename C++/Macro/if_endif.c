#include <stdio.h>
#include <stdlib.h>

#define PLATFORM 0
#if (PLATFORM == 0)
int main()
{
    printf("安卓平台的代码\n");
    system("pause");
    return 0;
}
#endif

#if (PLATFORM == 1)
int main()
{
    printf("windows 平台的代码\n");
    system("pause");
    return 0;
}
#endif