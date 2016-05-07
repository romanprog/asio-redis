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

int main () {


    std::vector<redis::srv_endpoint> master_pool;
    std::vector<redis::srv_endpoint> slave_pool;
    master_pool.emplace_back("127.0.0.1", 6379);
    slave_pool.emplace_back("127.0.0.1", 6379);
    redis::client cl;
    cl.run_thread_worker();
    std::future<asio::error_code> conn_f = cl.future_connect(master_pool, slave_pool);
    conn_f.wait();

    auto query_handler = [](redis::rds_err er_, const redis::resp_data & result)
    {
        std::cout << result.sres << std::endl;
    };

    cl.async_send("get stest", query_handler);

    if (conn_f.get())
        std::cout << "Bad!" <<std::endl;
    else
        std::cout << "Connected!" <<std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    int i = 1000;

}
