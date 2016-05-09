#include <random>
#include <iostream>
#include <functional>
#include <memory>
#include <map>
#include <algorithm>
#include <string>
#include <iostream>
#include <thread>

#include "../include/asio-redis.hpp"
#include "../include/threadsafe/tests_lab.hpp"

std::chrono::high_resolution_clock::time_point t_begin, t_end;

int main () {


#if 1
    std::vector<redis::srv_endpoint> master_pool;
    std::vector<redis::srv_endpoint> slave_pool;
    master_pool.emplace_back("127.0.0.1", 6379);
    slave_pool.emplace_back("127.0.0.1", 6379);
    redis::client cl;
    cl.run_thread_worker();
    std::future<asio::error_code> conn_f = cl.future_connect(master_pool, slave_pool);
    conn_f.wait();
    if (conn_f.get())
        std::cout << "Bad!" <<std::endl;
    else
        std::cout << "Connected!" <<std::endl;

    auto query_handler = [](redis::rds_err er_, const redis::resp_data & result)
    {
        if (result.ires == 1000000) {
            t_end = std::chrono::high_resolution_clock::now();
            std::chrono::high_resolution_clock::duration tm   = t_end - t_begin;
            std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(tm).count() << std::endl;
        }
    };

    t_begin = std::chrono::high_resolution_clock::now();

    cl.async_send("set test 0", query_handler);

    for (int i = 0; i < 1000000; ++i)
        cl.async_send("incr test", query_handler);


    std::this_thread::sleep_for(std::chrono::seconds(2));

#endif
    exit(0);
    redis::threadsafe::lab::queue_fast<int, std::mutex, redis::threadsafe::spin_lock> int_queue2;
    redis::threadsafe::queue<int> int_queue;
    redis::threadsafe::lab::queue_fast<int> int_queue1;
    srand(time(NULL));

    auto filler = [&int_queue]()
    {
        for (int i = 0; i < 1000000; ++i) {
            int_queue.push(i);
        }
    };

    auto poper = [&int_queue]()
    {
        int pop_data;
        for (int i = 0; i < 1000000; ++i) {
            int_queue.get_and_pop(pop_data);
        }
    };

    t_begin = std::chrono::high_resolution_clock::now();
    std::thread t1{filler};
    filler();
    std::thread t2{poper};
    std::thread t3{poper};
    t1.join();
    t2.join();
    t3.join();

    t_end = std::chrono::high_resolution_clock::now();
    std::chrono::high_resolution_clock::duration tm   = t_end - t_begin;
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(tm).count() << std::endl;
    t_begin = std::chrono::high_resolution_clock::now();

    t_end = std::chrono::high_resolution_clock::now();
    tm   = t_end - t_begin;
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(tm).count() << std::endl;
}




