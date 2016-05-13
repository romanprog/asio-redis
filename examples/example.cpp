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
#include "../utils/h_strings.hpp"
#include "../utils/profiler.hpp"

std::chrono::high_resolution_clock::time_point t_begin, t_end;
std::atomic<int> qcounter{0};
unsigned loops_count {100000};

int main () {

    auto buff_q_handler = [](redis::rds_err er, const redis::resp_data & res)
    {
        // std::cout << res.sres << std::endl;
        ++qcounter;
        if (qcounter == loops_count - 1)
        {
            t_end = std::chrono::high_resolution_clock::now();
            std::chrono::high_resolution_clock::duration tm   = t_end - t_begin;
            long mls = std::chrono::duration_cast<std::chrono::milliseconds>(tm).count();
            double qps = loops_count*1000/(mls?mls:mls+1);
            std::cout << "Loops: " << loops_count  << ", time: " << mls << "ms, " << qps << "qps" << std::endl;
            for (auto & tp : profiler::global().get_list())
            {
                std::cout << tp.first << " " << tp.second << std::endl;
            }
        }
    };
    using namespace redis;
    std::shared_ptr<std::string> _direct_buff = std::make_shared<std::string>("Some text");
    hstrings::rand_str(*_direct_buff, 1000);
    buff::output_adapter<std::string> _adapted_buffer(_direct_buff);
    query<cmd::incr> _set_cmd(buff_q_handler, "test");
    query<cmd::hash::hexists> test_query(buff_q_handler, "test");
    query<cmd::incr, buff::direct_write_buffer> dw_q(buff_q_handler, "test");


#if 1
    redis::client cl;
    cl.run_thread_worker();
    std::future<asio::error_code> conn_f = cl.future_connect("127.0.0.1", 6379);
    conn_f.wait();


    if (conn_f.get())
        std::cout << "Bad!" <<std::endl;
    else
        std::cout << "Connected!" <<std::endl;
    t_begin = std::chrono::high_resolution_clock::now();


    for (int i = 0; i < loops_count; ++i) {
        // std::shared_ptr<std::string> _direct_buff = std::make_shared<std::string>();
        // hstrings::rand_str(*_direct_buff, 1000);
        // buff::output_adapter<std::string> _adapted_buffer(_direct_buff);
        cl.async_send(dw_q);
    }
//        cl.async_send(dw_q);

    // cl.async_send(dw_q);

//    t11.join();
    std::this_thread::sleep_for(std::chrono::seconds(6));

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




