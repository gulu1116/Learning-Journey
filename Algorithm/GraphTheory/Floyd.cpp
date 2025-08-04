#include <iostream>
#include <cstring>
#include <algorithm>

using namespace std;

const int N = 210, INF = 1e9;  // N 表示最大节点数，INF 是一个大数，表示不可达

int n, m, Q;  // n: 节点数，m: 边数，Q: 查询次数
int d[N][N];  // d[i][j] 存储从节点 i 到节点 j 的最短路径

/**
 * @brief 使用 Floyd-Warshall 算法计算所有节点对之间的最短路径
 */
void floyd()
{
    // 三重循环，依次更新每对节点之间的最短路径
    for (int k = 1; k <= n; k++)  // 遍历所有的中间节点 k
        for (int i = 1; i <= n; i++)  // 遍历所有起点 i
            for (int j = 1; j <= n; j++)  // 遍历所有终点 j
                d[i][j] = min(d[i][j], d[i][k] + d[k][j]);  // 如果通过 k 使路径更短，则更新 d[i][j]
}

int main()
{
    cin >> n >> m >> Q;
    
    // 初始化所有节点对的最短路径为无穷大，除去 i == j 的情况（自己到自己为 0）
    for (int i = 1; i <= n; i++)
        for (int j = 1; j <= n; j++)
            if (i == j) d[i][j] = 0;  // 自己到自己，最短路径为 0
            else d[i][j] = INF;  // 其他节点初始化为无穷大，表示暂时不可达
    
    while (m--)
    {
        int a, b, w;
        cin >> a >> b >> w;
        
        // 对每条边，只保留权重最小的边（避免存在重复边的情况）
        d[a][b] = min(d[a][b], w);
    }

    floyd();
    
    // 处理查询
    while (Q--)
    {
        int a, b;
        cin >> a >> b;  // 输入查询节点对 (a, b)
        
        // 如果 d[a][b] 仍然为无穷大，表示 a 到 b 不可达，输出 "impossible"
        if (d[a][b] > INF / 2) puts("impossible");
        else cout << d[a][b] << endl;  // 否则输出最短路径
    }
    
    return 0;
}