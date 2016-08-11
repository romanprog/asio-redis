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
#include "../utils/h_strings.hpp"
#include "../utils/profiler.hpp"

// Variables for benchmark.
std::atomic<int> qcounter{0};
unsigned loops_count {100000};

int main () {
    using namespace redis;

    // Benchmark lambda for redis callback.
    auto buff_q_handler = [](redis::rds_err er, const redis::resp_data & res)
    {
        ++qcounter;
        if (qcounter == loops_count - 1)
        {
            profiler::global().checkpoint("qps");
            double mls = profiler::global().get_duration("qps");
            double qps = (static_cast<double>(loops_count)/(mls?mls:mls+1))*1000000;
            std::cout << "Loops: " << loops_count  << ", time: " << mls << "mks, " << qps << "qps" << std::endl;
        }
    };

    // Test string, that will used as direct buffer.
    std::string _data_for_save;
    // Fill string to random data. Size - 4K.
    hstrings::rand_str(_data_for_save, 4000);
    // Create query over the string, using buffer adapter.
    query<cmd::set, buff::direct_write_buffer> dw_q("text_test1", buff::output_adapter<std::string>(_data_for_save));

    // Main client.
    redis::client cl;
    // Try connect to database, using future function and one master endpoint.
    auto conn_f = cl.future_connect("127.0.0.1", 6379);
    conn_f.wait();

    // get method return asio::error_code, check for error.
    if (conn_f.get()) {
        // asio::error_code != 0, connection error.
        throw std::logic_error("Connection error");
    }
    else {
        // Connected.
        std::cout << "Connected!" <<std::endl;
    }

//    query<cmd::incr> incr_query("test");
      profiler::global().startpoint();
//    auto fut = cl.future_send(dw_q);
//    fut.wait();

    for (int i = 0; i < loops_count; ++i) {
        cl.async_send(dw_q , buff_q_handler);
    }

    std::this_thread::sleep_for(std::chrono::seconds(4));

//#endif

}




