

/*
    树和图的存储（两种）—— 树是无环连通图

    有向图和无向图：a-->b  a--b
        对于每一条无向边： a-->b  b-->a

    有向图：
        1. 邻接矩阵：g[a][b] 存储 a-->b 的权重/布尔值
            适合稠密图，空间复杂度：O(n^2)
        2. 邻接表：
            每个结点开一个单链表
                存储可以到达的点，次序无关紧要，一般头插法


    树和图的遍历：dfs/bfs

*/

#include <bits/stdc++.h>

using namespace std;

const int N = 100010, M = 2 * N;

int n, m;
int h[N], e[M], ne[M], idx;
bool st[N];
int ans = N;
//h[N]：邻接表头数组，存储每个节点的出度边。
//e[M]：存储边的终点。
//ne[M]：存储下一条边的索引。
//idx：边的索引。
//st[N]：标记数组，表示节点是否被访问过。
//ans：存储最终答案，初始设为 N（即 100010）。 


// a --> b
void add(int a, int b) 
{
    e[idx] = b;
    ne[idx] = h[a];
    h[a] = idx++;
}

int dfs(int u)
{
    st[u] = true;  // 标记一下已经搜过

    int sum = 1, res = 0;
    // sum 记录以 u 为根的子树的大小
    // res 记录当前节点删除后最大剩余连通块的大小

    for (int i = h[u]; i != -1; i = ne[i])
    {
        int j = e[i];

        // 如果 j 未访问，递归 dfs(j)，返回子树大小 s 
        if (!st[j])
        {
            int s = dfs(j);
            res = max(res, s);  //子树中最大块的大小
            sum += s;
        }
    }

    res = max(res, n - sum);

    ans = min(ans, res);

    return sum;
}

int main() 
{
    cin >> n;
    memset(h, -1, sizeof h);

    // 读入 n-1 条边
    for (int i = 0; i < n - 1; i++) 
    {
        int a, b;
        cin >> a >> b;
        add(a, b), add(b, a);
    }

    dfs(1);

    cout << ans << endl;

    return 0;
    
}