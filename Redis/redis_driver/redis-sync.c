#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hiredis/hiredis.h>

int main() {
    unsigned int j, isunix = 0;
    redisContext *c;
    redisReply *reply;
    const char *hostname = "127.0.0.1";

    int port = 6379;

    struct timeval timeout = { 1, 500000 }; // 1.5 seconds

    c = redisConnectWithTimeout(hostname, port, timeout);

    if (c == NULL || c->err) {
        if (c) {
            printf("Connection error: %s\n", c->errstr);
            redisFree(c);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        exit(1);
    }

    int roleid = 10001;
    reply = redisCommand(c, "hgetall role:%d", roleid);
    if (reply->type != REDIS_REPLY_ARRAY) {
        printf("reply error: %s\n", reply->str);
    } else {
        printf("reply:number of elements=%lu\n", reply->elements);
        for (size_t i = 0; i < reply->elements; i++) {
            printf("\t %lu : %s\n", i, reply->element[i]->str);
        }
    }
    freeReplyObject(reply);

    /* Disconnects and frees the context */
    redisFree(c);

    return 0;
}
