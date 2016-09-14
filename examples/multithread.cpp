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


    auto buff_q_handler = [](redis::rds_err er, const redis::resp_data & res)
    {
        ++qcounter;
        if (qcounter == loops_count - 1)
        {
            profiler::global().checkpoint("qps");
            double mls = profiler::global().get_duration("qps");
            // long
            double qps = (static_cast<double>(loops_count)/(mls?mls:mls+1))*1000000;
            std::cout << "Loops: " << loops_count  << ", time: " << mls/1000000 << "sec, " << qps << "qps" << std::endl;
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

    query<cmd::incr> incr_query("test");
    profiler::global().startpoint();

    auto funct = [&incr_query, &cl, &buff_q_handler]()
    {
        for (int i = 0; i < loops_count/3; ++i) {
                cl.async_send(incr_query , buff_q_handler);
        }

    };

    // Run queryes in 3 threads.
    std::thread t1(funct);
    std::thread t2(funct);
    funct();

    t1.join();
    t2.join();

}




