#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX_PROCESSES 5  // 最大进程数
#define MAX_RESOURCES 3  // 资源种类数

// 全局变量定义
int Max[MAX_PROCESSES][MAX_RESOURCES] = {
    {7, 5, 3}, {3, 2, 2}, {9, 0, 2}, {2, 2, 2}, {4, 3, 3}
};

int Allocation[MAX_PROCESSES][MAX_RESOURCES] = {
    {0, 1, 0}, {2, 0, 0}, {3, 0, 2}, {2, 1, 1}, {0, 0, 2}
};

int Need[MAX_PROCESSES][MAX_RESOURCES];
int Available[MAX_RESOURCES] = {3, 3, 2};
int safeArray[MAX_PROCESSES];    // 安全序列 
int n = 5, m = 3;               // 进程数和资源种类数

sem_t sem;  // 信号量，用于进程同步

// 安全性检查函数
bool isSafe()
{
    int Work[MAX_RESOURCES];
    int i, j;
    
    // 初始化工作向量为当前可用资源
    for (i = 0; i < m; i++) 
        Work[i] = Available[i];    
    
    // 进行试探分配时进程完成的标志 
    bool FinishLocal[MAX_PROCESSES] = {false};
    
    int safeCount = 0;  // 安全序列中的进程数量 
    
    while (true)
    {
        // 检查每次循环是否能找到一个可以顺利完成的进程
        bool found = false;        
        for (i = 0; i < n; i++)
        {
            if (!FinishLocal[i])
            {
                // 判断进程i是否可以顺利完成
                bool canFinish = true;
                for (j = 0; j < m; j++)
                {
                    // 如果进程i需要的资源大于可用资源
                    if (Need[i][j] > Work[j])    
                    {
                        canFinish = false;
                        break;
                    }
                }
                
                // 进程可以完成，释放分配给该进程的资源
                if (canFinish)
                {
                    for (j = 0; j < m; j++) 
                        Work[j] += Allocation[i][j]; 
                    
                    FinishLocal[i] = true;
                    // 将能够顺利完成的进程加入安全序列
                    safeArray[safeCount++] = i;    
                    found = true;
                    break; 
                } 
            }
        }
        
        // 如果没有进程可以完成，跳出循环
        if (!found) break; 
    }
    
    // 检查是否所有进程都完成
    for (i = 0; i < n; i++)
        if (!FinishLocal[i]) 
            return false;
    
    return true; 
}

// 请求和分配资源函数
bool requestResources(int processId, int Request[MAX_RESOURCES])
{
    int i;
    // 步骤1: 确认请求小于等于Need
    for (i = 0; i < m; i++)
    {
        if (Request[i] > Need[processId][i])
        {
            printf("请求错误！进程 %d 请求超出它的需求！\n", processId);
            return false; 
        }
    }
    
    // 步骤2: 确认请求小于等于Available
    for (i = 0; i < m; i++)
    {
        if (Request[i] > Available[i])
        {
            printf("没有足够的资源，请等待！\n");
            return false;
        }
    }
    
    // 步骤3: 假设资源分配给进程
    for (i = 0; i < m; i++)
    {
        Available[i] -= Request[i];
        Allocation[processId][i] += Request[i];
        Need[processId][i] -= Request[i]; 
    }
    
    // 步骤4: 检查系统是否安全
    if (isSafe())
    {
        printf("系统处于安全状态\n");
        
        // 输出安全序列
        printf("当前安全序列：");
        for (i = 0; i < n; i++) 
            printf("P%d   ", safeArray[i]);
        printf("\n");
        
        return true; 
    }
    else  // 恢复资源分配
    {
        for (i = 0; i < m; i++)
        {
            Available[i] += Request[i];
            Allocation[processId][i] -= Request[i];
            Need[processId][i] += Request[i]; 
        }
        printf("该请求会导致系统处于不安全状态！\n");
        return false; 
    }
}

// 模拟进程请求资源函数
void* processRequest(void* arg)
{
    int processId = *(int*)arg;
    int Request[MAX_RESOURCES];
    
    int i;
    printf("请输入进程 %d 请求的资源数量：", processId);
    for (i = 0; i < m; i++) 
        scanf("%d", &Request[i]);
    
    sem_wait(&sem);  // 获取信号量
    
    if(requestResources(processId, Request))
        printf("进程 %d 成功获得资源！\n", processId);
    else 
        printf("进程 %d 请求资源失败！\n", processId); 
    
    sem_post(&sem);  // 释放信号量
    return NULL;
}

int main()
{
    int i, j;
    
    // 计算Need矩阵
    for (i = 0; i < n; i++)
        for (j = 0; j < m; j++)
            Need[i][j] = Max[i][j] - Allocation[i][j];
            
    // 显示初始资源分配情况
    printf("最大需求矩阵Max：\n");
    for (i = 0; i < n; i++)
    {
        for (j = 0; j < m; j++) 
            printf("\t%d", Max[i][j]);
        printf("\n");
    }
    
    printf("\n分配矩阵Allocation：\n");
    for (i = 0; i < n; i++)
    {
        for (j = 0; j < m; j++) 
            printf("\t%d", Allocation[i][j]);
        printf("\n");
    }
    
    printf("\n进程剩余需求矩阵Need：\n");
    for (i = 0; i < n; i++)
    {
        for (j = 0; j < m; j++) 
            printf("\t%d", Need[i][j]);
        printf("\n");
    }
    
    // 初始化信号量
    sem_init(&sem, 0, 1);
    
    // 主循环，处理用户请求
    while(true)
    {
        char s[5];
        printf("\n是否有进程请求资源？yes or no：");
        scanf("%s", s);
        
        if (strcmp(s, "no") == 0) 
            break;
        
        int processId;
        printf("请输入请求资源的进程号（0~4）：");
        scanf("%d", &processId);
        
        if (processId < 0 || processId >= MAX_PROCESSES)
        {
            printf("进程号无效，请重新输入！\n");
            continue;
        }
        
        // 创建线程处理资源请求
        pthread_t thread;
        pthread_create(&thread, NULL, processRequest, &processId);
        pthread_join(thread, NULL);
    }
    
    // 销毁信号量
    sem_destroy(&sem);
    return 0;
}