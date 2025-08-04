#include <iostream>
#include <cstring>
#include <algorithm>
#include <queue>

using namespace std;

typedef pair<int, int> pii;

const int N = 100010;

int n, m;
int h[N], w[N], e[N], ne[N], idx;
int dist[N], cnt[N];
bool st[N];

void add(int a, int b, int c)
{
    e[idx] = b, w[idx] = c, ne[idx] = h[a], h[a] = idx++;
}


/**
 * @brief 使用 SPFA 算法计算最短路径
 * 
 * @return int 如果存在负权环或无法到达 n，返回 -1，否则返回从 1 到 n 的最短距离
 */
bool spfa()
{
    memset(dist, 0x3f, sizeof dist);
    dist[1] = 0;

    queue<int> q;
    q.push(1);
    st[1] = true;

    while (q.size())
    {
        int t = q.front();
        q.pop();

        st[t] = false;  // 将 t 从队列中移除

        // 遍历 t的所有邻接点 
        for (int i = h[t]; i != -1; i = ne[i])
        {
            int j = e[i];

            // 如果通过 t 到 j 的路径更短，更新 dist[j]
            if (dist[j] > dist[t] + w[i])
            {
                dist[j] = dist[t] + w[i];
                cnt[j] = cnt[t] + 1;  // 源点到点 j 最短距离经过的边数+1 

                // 如果松弛次数达到节点数 n，说明图中存在负权环 
                if (cnt[j] > n) return true;

                // 如果 j 还没有加入队列
                if (!st[j])
                {
                    q.push(j);
                    st[j] = true;  // 标记 j 已经加入队列
                }
            }
        }        
    }

    return false;
}

int main()
{
    cin >> n >> m;

    memset(h, -1, sizeof h);

    while (m--)
    {
        int a, b, c;
        cin >> a >> b >> c;
        add(a, b, c);
    }

    int t = spfa();

    if (spfa()) puts("Yes");
    else puts("No");

    return 0;
}