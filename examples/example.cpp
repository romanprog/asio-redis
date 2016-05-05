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

    asio::io_service io;
    std::shared_ptr<asio::strand> _str_p = std::make_shared<asio::strand>(io);
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


}
