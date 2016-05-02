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
    using namespace redis::threadsafe;
    //redis::query<redis::cmd::blpop> cmd;
    std::mutex mux;

    int test_counter {0};
    std::string str{"0"};

    std::thread t2([&](){
        for (int i = 0; i < 1000; ++i) {
            //std::this_thread::sleep_for(std::chrono::milliseconds(1));

            std::lock_guard<std::mutex>  lg1(mux);
            ++test_counter;
            str = "Some string counter ";
            str += std::to_string(test_counter);
            str += " end of str";

        }
        std::cout << "thread free" << std::endl;

    });

    int tc2{0};

    for (int i = 0; i < 100000000; ++i)
    {
        std::lock_guard<std::mutex> lg(mux);
        if (tc2 != test_counter) {
            std::cout << tc2 << " " << str << " " << std::endl;
            tc2 = test_counter;
        }
    }

    t2.join();
    std::cout << "main free " << test_counter << std::endl;
}
