
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
test_string_cmd(redisAsyncContext *c) {
    printf("===============> begin test string\n");
    redisAsyncCommand(c, dumpReply, "set mark 1000", "set mark 1000");
    redisAsyncCommand(c, dumpReply, "get mark", "get mark");
    redisAsyncCommand(c, dumpReply, "del mark", "del mark");
}

void
test_list_cmd(redisAsyncContext *c) {
    printf("===============> begin test list\n");
    redisAsyncCommand(c, dumpReply, "lpush list", "lpush list %s %s %s", "darren", "mark", "1000");
    redisAsyncCommand(c, dumpReply, "lpop list", "lpop list");
    redisAsyncCommand(c, dumpReply, "lrange list 0 -1", "lrange list 0 -1");
}

void
test_hash_cmd(redisAsyncContext *c) {
    printf("===============> begin test hash\n");
    redisAsyncCommand(c, dumpReply, "hmset role:10001", "hmset role:10001 name mark age 31 sex male");
    redisAsyncCommand(c, dumpReply, "hgetall role:10001", "hgetall role:10001");
    redisAsyncCommand(c, dumpReply, "hset role:10001 age 32","hset role:10001 age 32");
    redisAsyncCommand(c, dumpReply, "hgetall role:10001", "hgetall role:10001");
}

void
test_set_cmd(redisAsyncContext *c) {
    printf("===============> begin test set\n");
    redisAsyncCommand(c, dumpReply, "sadd teachers", "sadd teachers mark darren king");
    redisAsyncCommand(c, dumpReply, "smembers teachers", "smembers teachers");
    redisAsyncCommand(c, dumpReply, "spop teachers", "spop teachers");
    redisAsyncCommand(c, dumpReply, "smembers teachers", "smembers teachers");
}

void
test_zset_cmd(redisAsyncContext *c) {
    printf("===============> begin test zset\n");
    redisAsyncCommand(c, dumpReply, "zadd ranks", "zadd ranks 80 mark 90 darren 100 king");
    redisAsyncCommand(c, dumpReply, "zrange ranks", "zrange ranks 0 -1");
    redisAsyncCommand(c, dumpReply, "zincrby ranks", "zincrby ranks 11 mark");
    redisAsyncCommand(c, dumpReply, "zrange ranks", "zrange ranks 0 -1");
}

void
test_lua_cmd(redisAsyncContext *c) {
    printf("===============> begin test lua\n");
    redisAsyncCommand(c, dumpReply, "set score 2", "set score 2");

    sds dval = sdsnew("local val = redis.call('get', 'score');if val then redis.call('set', 'score', 2*val); return 2*val; end;return 0;");
    char *doubleValueScriptArgv[] = {
        (char*)"eval",
        dval,
        (char*)"1",
        (char*)"score",
    };
    for (int i = 0; i < 4; i++)
        redisAsyncCommandArgv(c, dumpReply, "eval script double value", 4, (const char **)doubleValueScriptArgv, NULL);
    sdsfree(dval);
}

void
connectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        stop_eventloop(R);
        return;
    }
    printf("Connected...\n");
    // test_string_cmd((redisAsyncContext *)c);
    test_lua_cmd((redisAsyncContext *)c);
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