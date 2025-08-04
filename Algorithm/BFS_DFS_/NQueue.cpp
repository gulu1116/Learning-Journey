#include <bits/stdc++.h>

using namespace std;

const int N = 20;

int n;
char g[N][N];
bool col[N], dg[N], udg[N];

void dfs(int u) {

    if (u == n) {
        for (int i = 0; i < n; i++) cout << g[i] << endl;
        cout << endl;
        return;
    }

    for (int i = 0; i < n; i++)  {
        if (!col[i] && !dg[i - u + n] && !udg[i + u]) {
            g[u][i] = 'Q';
            col[i] = dg[i - u + n] = udg[i + u] = true;
            dfs(u + 1);
            col[i] = dg[i - u + n] = udg[i + u] = false;
            g[u][i] = '.';
        }
    }
}

int main() {
    cin >> n;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++)
            g[i][j] = '.'
    }

    dfs(0);

    return 0;
}