#include <hiredis_cluster/adapters/libevent.h>
#include <hiredis_cluster/hircluster.h>
#include <event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>

int64_t g_taskid = 0;

#define MAX_KEY 10

static int64_t hi_msec_now() {
    int64_t msec;
    struct timeval now;
    int status;
    status = gettimeofday(&now, NULL);
    if (status < 0) {
        return -1;
    }
    msec = (int64_t)now.tv_sec * 1000LL + (int64_t)(now.tv_usec / 1000LL);
    return msec;
}

static int _vscnprintf(char *buf, size_t size, const char *fmt, va_list args) {
    int n;
    n = vsnprintf(buf, size, fmt, args);
    if (n <= 0) {
        return 0;
    }
    if (n <= (int)size) {
        return n;
    }
    return (int)(size-1);
}

static int _scnprintf(char *buf, size_t size, const char *fmt, ...) {
    va_list args;
    int n;
    va_start(args, fmt);
    n = _vscnprintf(buf, size, fmt, args);
    va_end(args);
    return n;
}

void connectCallback(const redisAsyncContext *ac, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", ac->errstr);
        return;
    }
    printf("Connected to %s:%d\n", ac->c.tcp.host, ac->c.tcp.port);
}

void disconnectCallback(const redisAsyncContext *ac, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", ac->errstr);
        return;
    }
    printf("Disconnected from %s:%d\n", ac->c.tcp.host, ac->c.tcp.port);
}

void addTaskCallback(redisClusterAsyncContext *cc, void *r, void *privdata) {
    redisReply *reply = (redisReply *)r;
    if (reply == NULL) {
        if (cc->errstr) {
            printf("errstr: %s\n", cc->errstr);
        }
        return;
    }

    int64_t now = hi_msec_now() / 10;
    printf("add task success reply: %lld now=%ld\n", reply->integer, now);
}

int addTask(redisClusterAsyncContext *cc, char *desc) {
    /* 转化为厘米秒 */
    int64_t now = hi_msec_now() / 10;
    g_taskid++;
    
    /* key */
    char key[256] = {0};
    int len = _scnprintf(key, 255, "task_group:%ld", g_taskid % MAX_KEY);
    key[len] = '\0';
    
    /* member */
    char mem[1024] = {0};
    len = _scnprintf(mem, 1023, "task:%ld:%s", g_taskid, desc);
    mem[len] = '\0';
    
    int status;
    status = redisClusterAsyncCommand(cc, addTaskCallback, "",
                                      "zadd %s %ld %s", key, now+500, mem);

    printf("redisClusterAsyncCommand:zadd %s %ld %s\n", key, now+500, mem);
    if (status != REDIS_OK) {
        printf("error: err=%d errstr=%s\n", cc->err, cc->errstr);
    }
    return 0;
}

void stdio_callback(struct bufferevent *bev, void *arg) {
    redisClusterAsyncContext *cc = (redisClusterAsyncContext *)arg;
    struct evbuffer *evbuf = bufferevent_get_input(bev);
    char *msg = evbuffer_readln(evbuf, NULL, EVBUFFER_EOL_LF);
    if (!msg) return;

    if (strcmp(msg, "quit") == 0) {
        printf("safe exit!!!\n");
        exit(0);
        return;
    }
    if (strlen(msg) > 1024-5-13-1) {
        printf("[err]msg is too long, try again...\n");
        return;
    }

    addTask(cc, msg);
    printf("stdio read the data: %s\n", msg);
}

int main(int argc, char **argv) {
    printf("Connecting...\n");
    redisClusterAsyncContext *cc =
        redisClusterAsyncConnect("127.0.0.1:7006", HIRCLUSTER_FLAG_NULL);
    printf("redisClusterAsyncContext...\n");
    if (cc && cc->err) {
        printf("Error: %s\n", cc->errstr);
        return 1;
    }

    struct event_base *base = event_base_new();
    redisClusterLibeventAttach(cc, base);
    redisClusterAsyncSetConnectCallback(cc, connectCallback);
    redisClusterAsyncSetDisconnectCallback(cc, disconnectCallback);

    // nodeIterator ni;
    // initNodeIterator(&ni, cc->cc);
    // cluster_node *node;
    // while ((node = nodeNext(&ni)) != NULL) {
    //     printf("node %s:%d role:%d pad:%d\n", node->host, node->port, node->role, node->pad);
    // }
    struct bufferevent *ioev = bufferevent_socket_new(base, 0, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(ioev, stdio_callback, NULL, NULL, cc);
    bufferevent_enable(ioev, EV_READ | EV_PERSIST);

    printf("Dispatch..\n");
    event_base_dispatch(base);

    printf("Done..\n");
    redisClusterAsyncFree(cc);
    event_base_free(base);
    return 0;
}

// 需要安装 hiredis-cluster libevent
// gcc producer.c -o producer -levent -lhiredis_cluster -lhiredis -lhiredis_ssl
