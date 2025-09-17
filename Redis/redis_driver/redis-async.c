
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/sds.h>

#include "reactor.h"
#include "adapter_async.h"

static reactor_t *R;

char *rtype[] = {
    "^o^",
    "STRING",
    "ARRAY",
    "INTEGER",
    "NIL",
    "STATUS",
    "ERROR",
    "DOUBLE",
    "BOOL",
    "MAP",
    "SET",
    "ATTR",
    "PUSH",
    "BIGNUM",
    "VERB",
};

void dumpReply(struct redisAsyncContext *c, void *r, void *privdata) {
    redisReply *reply = (redisReply*)r;
    switch (reply->type) {
    case REDIS_REPLY_STATUS:
    case REDIS_REPLY_STRING:
        printf("[req = %s]reply:(%s)%s\n", (char*)privdata, rtype[reply->type], reply->str);
        break;
    case REDIS_REPLY_NIL:
        printf("[req = %s]reply:(%s)nil\n", (char*)privdata, rtype[reply->type]);
        break;
    case REDIS_REPLY_INTEGER:
        printf("[req = %s]reply:(%s)%lld\n", (char*)privdata, rtype[reply->type], reply->integer);
        break;
    case REDIS_REPLY_ARRAY:
        printf("[req = %s]reply(%s):number of elements=%lu\n", (char*)privdata, rtype[reply->type], reply->elements);
        for (size_t i = 0; i < reply->elements; i++) {
            printf("\t %lu : %s\n", i, reply->element[i]->str);
        }
        break;
    case REDIS_REPLY_ERROR:
        printf("[req = %s]reply(%s):err=%s\n", (char*)privdata, rtype[reply->type], reply->str);
        break;
    default:
        printf("[req = %s]reply(%s)\n", (char*)privdata, rtype[reply->type]);
        break;
    }
}

void
connectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        stop_eventloop(R);
        return;
    }
    printf("Connected...\n");
    redisAsyncCommand((redisAsyncContext *)c, dumpReply, 
        "hmset role:10001", 
        "hmset role:10001 name mark age 31 sex male");
    int a = 10;
    redisAsyncCommand((redisAsyncContext *)c, dumpReply, "hgetall role:10001", "hgetall role:10001");
    // ....
}

void
disconnectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        stop_eventloop(R);
        return;
    }

    printf("Disconnected...\n");
    stop_eventloop(R);
}

int main(int argc, char **argv) {
    R = create_reactor();
    redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 6379);
    if (c->err) {
        /* Let *c leak for now... */
        printf("Error: %s\n", c->errstr);
        return 1;
    }
    redisAttach(R, c);
    
    redisAsyncSetConnectCallback(c, connectCallback);
    redisAsyncSetDisconnectCallback(c, disconnectCallback);

    eventloop(R);

    release_reactor(R);
    return 0;
}