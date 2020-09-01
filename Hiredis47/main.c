#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "..\..\redis\deps\hiredis\hiredis.h"
#include "..\..\redis\deps\hiredis\async.h"
#include "..\..\redis\deps\hiredis\adapters\libuv.h"

//#include <hiredis.h>
//#include <async.h>
//#include <adapters/libuv.h>

void getCallback(redisAsyncContext* c, void* r, void* privdata) {
    redisReply* reply = r;
    if (reply == NULL) return;
    printf("argv[%s]: %s\n", (char*)privdata, reply->str);

    /* Disconnect after receiving the reply to GET */
    redisAsyncDisconnect(c);
}

void connectCallback(const redisAsyncContext* c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Connected...\n");
}

void disconnectCallback(const redisAsyncContext* c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Disconnected...\n");
}

void test(redisAsyncContext* c) {
    struct fd_set* readable = malloc(sizeof(readable));
    memset(readable, 0, sizeof(readable));
    readable->fd_count = 1;
    readable->fd_array[0] = c->c.fd;

    char* buff = "set t 1";
    send(c->c.fd, buff, sizeof(buff), 0);
    struct timeval val;
    val.tv_sec = 1;
    val.tv_usec = 0;

    int result = select(
        0,
        readable,
        NULL,
        NULL,
        &val);

    if (result == SOCKET_ERROR) {
        printf("socket error");
    }
}

void connectSynchronous() {
    redisContext* c = redisConnect("127.0.0.1", 6379);
    char* buff = "SET t 1";
    redisReply* reply = (redisReply*)redisCommand(c, buff);
    exit(0);
}

int main(int argc, char** argv) {
    //signal(SIGPIPE, SIG_IGN);
    connectSynchronous();
    uv_loop_t* loop = uv_default_loop();
    HANDLE ioPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);

    redisAsyncContext* c = redisAsyncConnect("127.0.0.1", 6379);
    CreateIoCompletionPort(c->c.fd, ioPort, c->c.fd, 1);
    if (c->err) {
        /* Let *c leak for now... */
        printf("Error: %s\n", c->errstr);
        return 1;
    }

    //test(c);
    redisLibuvAttach(c, loop);
    redisAsyncSetConnectCallback(c, connectCallback);
    redisAsyncSetDisconnectCallback(c, disconnectCallback);
    redisAsyncCommand(c, NULL, NULL, "SET key %b", argv[argc - 1], strlen(argv[argc - 1]));
    redisAsyncCommand(c, getCallback, (char*)"end-1", "GET key");
    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}
