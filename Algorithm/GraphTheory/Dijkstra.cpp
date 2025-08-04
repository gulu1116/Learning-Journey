/*
    所有边为正，自环不会出现在最短路里
    重边只需要保留最短的边
*/

#include <iostream>
#include <cstring>
#include <algorithm>

using namespace std;

const int N = 510;
const int INF = 0x3f3f3f3f; // Define INF as a large value

int n, m;
int g[N][N];
int dist[N];
bool st[N];

int dijkstra()
{
    memset(dist, 0x3f, sizeof dist);
    dist[1] = 0;

    for (int i = 0; i < n; i++)
    {
        int t = -1;
        for (int j = 1; j <=n; j++)
            if (!st[j] && (t == -1 || dist[t] > dist[j]))
                t = j;

        st[t] = true;

        for (int j = 1; j <= n; j++)
            dist[j] = min(dist[j], dist[t] + g[t][j]);
    }

    return dist[n] == INF ? -1 : dist[n];
}

int main() 
{
    cin >> n >> m;

    memset(g, 0x3f, sizeof g);

    while (m--)
    {
        int a, b, c;
        cin >> a >> b >> c;
        g[a][b] = min(g[a][b], c);
    }

    int t = dijkstra();

    cout << t << endl;

    return 0;
}