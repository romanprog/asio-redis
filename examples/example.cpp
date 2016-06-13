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

auto empty_handler = [](redis::rds_err er, const redis::resp_data & res)
{
};

std::atomic<int> qcounter{0};
unsigned loops_count {1000000};

int main () {
    using namespace redis;

    std::string test_bulk {"$5\r\nROMER\r\n"};
    resp_proto::parse_string(test_bulk);
    std::cout << test_bulk << std::endl;

    exit(0);

//    auto buff_q_handler = [](redis::rds_err er, const redis::resp_data & res)
//    {
//        ++qcounter;
//        if (qcounter == loops_count - 1)
//        {
//            profiler::global().checkpoint("qps");
//            double mls = profiler::global().get_duration("qps");
//            // long
//            double qps = (static_cast<double>(loops_count)/(mls?mls:mls+1))*1000000;
//            std::cout << "Loops: " << loops_count  << ", time: " << mls << "mks, " << qps << "qps" << std::endl;
//        }
//    };

//    std::string _data_for_save;
//    hstrings::rand_str(_data_for_save, 4000);
    query<cmd::get> get_test("test");
//    query<cmd::set, buff::direct_write_buffer> dw_q("text_test1", buff::output_adapter<std::string>(_data_for_save));


// #if 1
    redis::client cl;
    auto conn_f = cl.future_connect("127.0.0.1", 6379);
    conn_f.wait();


    if (conn_f.get()) {
        throw std::logic_error("Connection error");
    }
    else {
        std::cout << "Connected!" <<std::endl;
    }

//    query<cmd::incr> incr_query("test");
//    profiler::global().startpoint();
//    auto fut = cl.future_send(dw_q);
//    fut.wait();

//    for (int i = 0; i < loops_count; ++i) {
//        cl.async_send(incr_query , buff_q_handler);
//    }
    auto q_fut = cl.future_send(get_test);
    q_fut.wait();
    auto res = q_fut.get();
    //        cl.async_send(dw_q);

    // cl.async_send(dw_q);

    //    t11.join();
    std::this_thread::sleep_for(std::chrono::seconds(2));

//#endif

}




