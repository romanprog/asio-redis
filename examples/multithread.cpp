#include <random>
#include <iostream>
#include <functional>
#include <memory>
#include <map>
#include <algorithm>
#include <string>
#include <iostream>
#include <thread>

#include "asio-redis.hpp"
#include "../include/utils/h_strings.hpp"
#include "../profiler/profiler.hpp"

auto empty_handler = [](redis::rds_err er, const redis::resp_data & res)
{
};

std::atomic<int> qcounter{0};
unsigned loops_count {3000000};

int main () {
    using namespace redis;


//    auto buff_q_handler = [](redis::rds_err er, const redis::resp_data & res)
//    {
//        ++qcounter;
//        if (qcounter == loops_count - 1)
//        {
//            profiler::global().checkpoint("qps");
//            double mls = profiler::global().get_duration("qps");
//            // long
//            double qps = (static_cast<double>(loops_count)/(mls?mls:mls+1))*1000000;
//            std::cout << "Loops: " << loops_count  << ", time: " << mls/1000000 << "sec, " << qps << "qps" << std::endl;
//        }
//    };

    auto buff_q_handler1 = [](redis::rds_err er, const redis::resp_data & res)
    {
        if (res.sres != "0")
            std::cout << "Error " << res.sres << std::endl;

    };
    auto buff_q_handler2 = [](redis::rds_err er, const redis::resp_data & res)
    {
        if (res.sres != "2")
            std::cout << "Error " << res.sres << std::endl;

    };
    auto buff_q_handler3 = [](redis::rds_err er, const redis::resp_data & res)
    {
        if (res.sres != "3")
            std::cout << "Error " << res.sres << std::endl;

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

    query<cmd::get> incr_query("test");
    query<cmd::get> incr_query2("test2");
    query<cmd::get> incr_query3("test3");
    profiler::global().startpoint();

    auto funct = [&incr_query, &cl, &buff_q_handler1]()
    {
        for (int i = 0; i < 500000; ++i) {
                cl.async_send(incr_query , buff_q_handler1);
        }

    };
    auto funct2 = [&incr_query2, &cl, &buff_q_handler2]()
    {
        for (int i = 0; i < 1500000; ++i) {
                cl.async_send(incr_query2 , buff_q_handler2);
        }

    };
    auto funct3 = [&incr_query3, &cl, &buff_q_handler3]()
    {
        for (int i = 0; i < 1000000; ++i) {
                cl.async_send(incr_query3 , buff_q_handler3);
        }

    };

    // Run queryes in 3 threads.
    std::thread t1(funct);
    std::thread t2(funct2);
    funct3();

    t1.join();
    t2.join();

}




