#include <bits/stdc++.h>

#define x first
#define y second 

using namespace std;

typedef pair<int, int> pii;

const int N = 100;

int n, m;
int g[N][N];
int d[N][N];

int bfs() {
    queue<pii> q;
    q.push({0, 0});

    memset(d, -1, sizeof d);
    d[0][0] = 0;

    int dx[4] = {-1, 0, 1, 0}, dy[4] = {0, 1, 0, -1};

    while (!q.empty()) {
        auto t = q.front();
        q.pop();

        for (int i = 0; i < 4; i++) {
            int a = t.x + dx[i], b = t.y + dy[i];
            if (a >= 0 && a < n && b >= 0 && b < m && g[a][b] == 0 && d[a][b] == -1) {
                d[a][b] = d[t.x][t.y] + 1;
                q.push({a, b});
            }
        }
    }

    return d[n-1][m-1];
}

int main() {
    cin >> n >> m;

    for (int i = 0; i < n; i ++) {
        for (int j = 0; j < m; j++)
            cin >> g[i][j];
    }

    cout << bfs() <<endl;

    return 0;
}