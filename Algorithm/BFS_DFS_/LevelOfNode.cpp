#include <cstring>
#include <iostream>
#include <algorithm>

using namespace std;

const int N = 100010;

int n, m;
int h[N], e[N], ne[N], idx;
int d[N], q[N];

void add(int a, int b)
{
	e[idx] = b, ne[idx] = h[a], h[a] = idx++;
}

int bfs()
{
	int hh = 0, tt = 0;
	q[0] = 1;	// 将起始节点 1 入队 
	
	memset(d, -1, sizeof d);
	
	d[1] = 0;	// 起始点为 1，距离为0 
	
	while (hh <= tt)
	{
		int t = q[hh++];
		
		// 遍历与 t 节点相连的所有邻居节点 
		for (int i = h[t]; i != -1; i = ne[i])
		{
			int j = e[i];	// j 是 t 节点的邻接节点 
			if (d[j] == -1)	// 距离-1表示邻接节点未访问 
			{
				d[j] = d[t] + 1;  // 访问节点 j，距离 +1 
				q[++tt] = j;		// 将邻接节点 j 入队 
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