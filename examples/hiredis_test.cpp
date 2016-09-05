#include <random>
#include <iostream>
#include <functional>
#include <memory>
#include <string>
#include <atomic>
#include <chrono>
#include <thread>

#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libev.h>
#include "../profiler/profiler.hpp"


std::atomic<int> qcounter{0};
unsigned loops_count {1000000};

void getCallback(redisAsyncContext *c, void *r, void *privdata) {

    redisReply *reply = static_cast<redisReply *>(r);
    if (reply == NULL) return;
    // printf("Connected...\n");
    ++qcounter;
    if (qcounter == loops_count - 1)
    {
        profiler::global().checkpoint("qps");
        double mls = profiler::global().get_duration("qps");
        // long
        double qps = (static_cast<double>(loops_count)/(mls?mls:mls+1))*1000000;
        std::cout << "Loops: " << loops_count  << ", time: " << mls << "mks, " << qps << "qps" << std::endl;
        redisAsyncDisconnect(c);
    }
}

void connectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error C: %s\n", c->errstr);
        return;
    }
    printf("Connected...\n");
}

void disconnectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error D: %s\n", c->errstr);
        return;
    }
    printf("Disconnected...\n");
}

int main () {

    signal(SIGPIPE, SIG_IGN);

    redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 6379);
    if (c->err) {
        /* Let *c leak for now... */
        printf("Error M: %s\n", c->errstr);
        return 1;
    }

    redisLibevAttach(EV_DEFAULT_ c);
    redisAsyncSetConnectCallback(c,connectCallback);
    redisAsyncSetDisconnectCallback(c,disconnectCallback);

    profiler::global().startpoint();
    for (int i = 0; i < loops_count; ++i)
        redisAsyncCommand(c, getCallback, NULL, "INC test");


    ev_loop(EV_DEFAULT_ 0);

    //std::this_thread::sleep_for(std::chrono::seconds(10));
}




