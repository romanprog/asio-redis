#include <random>
#include <iostream>
#include <functional>
#include <memory>
#include <map>
#include <algorithm>
#include <string>
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>

#include "asio-redis.hpp"
#include "../include/utils/h_strings.hpp"
#include "../profiler/profiler.hpp"

auto empty_handler = [](redis::rds_err er, const redis::resp_data & res)
{
};

std::atomic<int> qcounter{0};
unsigned loops_count {1000000};

// Condition tests
std::mutex mux;
std::condition_variable cond;
std::atomic<bool> cond_b {false};
int iii {0};

bool cond_f()
{
    if (cond_b) {
        ++iii;
        if (iii == 20) {
            iii = 0;
            cond_b = false;
        }
        return true;
    }
    return false;
}

int main () {

//    auto thread_func = []()
//    {
//        for (;;) {
//            std::unique_lock<std::mutex> lc(mux);
//            cond.wait(lc, cond_f);
//            // Some work.
//            std::thread::id tid = std::this_thread::get_id();
//            std::cout << tid << " Worked!" << std::endl;
//            lc.unlock();

//            std::this_thread::sleep_for(std::chrono::microseconds(500));
//        }
//    };

//    std::thread t1 {thread_func};
//    std::thread t2 {thread_func};
//    for (;;)
//    {
//        std::string i;
//        std::getline(std::cin, i);
//        std::lock_guard<std::mutex> lc {mux};
//        if (i == "yes")
//            cond_b = true;
//        cond.notify_all();
//    }

//    t1.join();
//    t2.join();
//    exit(0);
    using namespace redis;


    auto buff_q_handler = [](redis::rds_err er, const redis::resp_data & res)
    {
        ++qcounter;
        if (qcounter == loops_count - 1)
        {
            profiler::global().checkpoint("qps");
            double mls = profiler::global().get_duration("qps");
            // long
            double qps = (static_cast<double>(loops_count)/(mls?mls:mls+1))*1000000;
            std::cout << "Loops: " << loops_count  << ", time: " << mls << "mks, " << qps << "qps" << std::endl;
        }
    };

    redis::client cl;
    auto conn_f = cl.future_connect("127.0.0.1", 6379);
    conn_f.wait();


    if (conn_f.get()) {
        throw std::logic_error("Connection error");
    }
    else {
        std::cout << "Connected!" <<std::endl;
    }
    profiler::global().startpoint();
    for (int i = 1; i < 1000000; ++i)
        query<cmd::incr> incr_query("test", "fsadfsdaf");

    profiler::global().checkpoint("first");
    double mls = profiler::global().get_duration("first");
    double qps = (static_cast<double>(1000000)/(mls?mls:mls+1))*1000000;
    std::cout << "Loops: " << loops_count  << ", time: " << mls << "mks, " << qps << "qps" << std::endl;

    for (int i = 1; i < 1000000; ++i) {
        query<cmd::custom> set_test1("test", "test");
        int ik = 1;
    }

    profiler::global().checkpoint("second");

    mls = profiler::global().get_duration("second");
    qps = (static_cast<double>(1000000)/(mls?mls:mls+1))*1000000;
    std::cout << "Loops: " << loops_count  << ", time: " << mls << "mks, " << qps << "qps" << std::endl;


    query<cmd::set> incr_query("test");
    profiler::global().startpoint();


    for (int i = 0; i < loops_count/100; ++i) {
        for (int j = 0; j < 100; ++j)
            cl.async_send(incr_query , buff_q_handler);
        // std::this_thread::sleep_for(std::chrono::microseconds(10));
    }

}




