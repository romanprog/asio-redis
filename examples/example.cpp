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


//    profiler::global().startpoint();
//    for (int i = 0; i < loops_count; ++i) {
//        query<cmd::set> get_test(empty_handler, "any", "some text");
//    }

/*    profiler::global().checkpoint("qcreate");
    double mls = profiler::global().get_duration("qcreate");
    double qps = (static_cast<double>(loops_count)/(mls?mls:mls+1))*1000000;
    std::cout << "Loops: " << loops_count  << ", time: " << mls << "mks, " << qps << "cps" << std::endl;

    exit(0)*/;

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
    query<cmd::cluster::cl_countkeysinslot> ds("sadasd");
    std::cout << ds.as_string_ref() << std::endl;
    exit(0);

    std::shared_ptr<std::string> _direct_buff = std::make_shared<std::string>("Some text");
    hstrings::rand_str(*_direct_buff, 4000);
    buff::output_adapter<std::string> _adapted_buffer(_direct_buff);
    query<cmd::set> get_test("test", "1000");
    query<cmd::hash::hexists> test_query("test");
    // query<cmd::set, buff::direct_write_buffer> dw_q(buff_q_handler, "text_test", _adapted_buffer);


#if 1
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
    for (int i = 0; i < loops_count; ++i) {
        // std::shared_ptr<std::string> _direct_buff = std::make_shared<std::string>();
        // hstrings::rand_str(*_direct_buff, 1000);
        // buff::output_adapter<std::string> _adapted_buffer(_direct_buff);
        cl.async_send(query<cmd::set>("test", "1"), buff_q_handler);
    }
    auto q_fut = cl.future_send("set test 0");
    q_fut.wait();
    auto res = q_fut.get();
//        cl.async_send(dw_q);

    // cl.async_send(dw_q);

//    t11.join();
    std::this_thread::sleep_for(std::chrono::seconds(2));

#endif

}




