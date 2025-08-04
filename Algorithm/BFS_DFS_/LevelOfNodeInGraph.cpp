
/*

    题目要求求解从 1 号点到 n 号点的最短路径，
    给定的是一个有向图，所有边的长度均为 1。
    可以使用 广度优先搜索（BFS）来求解最短路径，
    因为在无权图中，BFS 能够保证求出的路径是最短的。

*/




#include <bits/stdc++.h>

using namespace std;

const int N = 100010, M = 2 * N;

int n, m;
int h[N], e[M], ne[M], idx;
int d[N];

void add(int a, int b)
{
    e[idx] = b, ne[idx] = h[a], h[a] = idx++;
}

int bfs()
{
    queue<int> q;
    memset (d, -1, sizeof d);
    q.push(1);
    d[1] = 0;

    while (!q.empty())
    {
        auto t = q.front();
        q.pop();

        for (int i = h[t]; i != -1; i = ne[i])
        {
            int j = e[i];
            if (d[j] == -1)
            {
                d[j] = d[t] + 1;
                q.push(j);
            }
        }
    }

    return d[n];
}

int main()
{
    cin >> n >> m;

    memset(h, -1, sizeof h);

    for (int i = 0; i < m; i++)
    {
        int a, b;
        cin >> a >> b;
        add(a, b);
    }

    cout << bfs() << endl;

    return 0;
}


