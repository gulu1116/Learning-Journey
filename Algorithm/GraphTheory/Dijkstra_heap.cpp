#include <iostream>
#include <algorithm>
#include <cstring>
#include <queue>

using namespace std;

#define x first 
#define y second

typedef pair<int, int> pii;

const int N = 100010;

int n, m;
int h[N], w[N], e[N], ne[N], idx;
int dist[N];
bool st[N];

void add(int a, int b, int c)
{
    e[idx] = b, w[idx] = c, ne[idx] = h[a], h[a] = idx++;
}

int dijkstra()
{
    memset(dist, 0x3f, sizeof dist);
    dist[1] = 0;

    priority_queue<pii, vector<pii>, greater<pii>> heap;
    heap.push({0, 1});

    while (heap.size())
    {
        auto t = heap.top();
        heap.pop();

        int ver = t.y, distance = t.x;
        if (st[ver]) continue;
        st[ver] = true;

        for (int i = h[ver]; i != -1; i = ne[i])
        {
            int j = e[i];

            if (dist[j] > distance + w[i])
            {
                dist[j] = distance + w[i];
                heap.push({dist[j], j});
            }
        }
    }

    return dist[n] == 0x3f3f3f3f ? -1 : dist[n];
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

    int t = dijkstra();
    cout << t << endl;

    return 0;
}