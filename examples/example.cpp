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
    spool<int> sp;
    sp.add_unit(0, 3);
    sp.add_unit(1, 5);
    sp.add_unit(2, 10);
    std::vector<unsigned> res{0,0,0};
    for (int i = 0; i < 1000; ++i)
    {
        // std::cout << sp.rand_unit() << std::endl;
        ++res[sp.rand_unit()];
    }
    for (auto & i : res)
        std::cout << i << std::endl;


}
