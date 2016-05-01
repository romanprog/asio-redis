#ifndef CONN_POOL_HPP
#define CONN_POOL_HPP
#include "types.hpp"
#include "thread_safe.hpp"
#include "unordered_map"

#include <asio.hpp>
#include <string>

namespace redis {

class conn;

using conn_ptr = std::shared_ptr<conn>;

class conn
{
public:
    conn(const strand_ptr & ev, std::string ip_, unsigned port_);
    void get(get_soc_callback cb_);
private:
    strand_ptr _ev_loop;
    threadsafe::conn_queue<soc_ptr> _cache;
    std::string _ip;
    unsigned _port;
};


class conn_pool
{
public:
    conn_pool(strand_ptr ev);
    // Interface sketches
    void add_master(const std::string & ip_, unsigned port_, unsigned priority_ = 100);
    void add_slave(const std::string & ip_, unsigned port_, unsigned priority_ = 100);

    void async_get_master(get_soc_callback cb_);
    void async_get_slave(get_soc_callback cb_);

private:
    strand_ptr _ev_loop;
    std::multimap<unsigned, conn_ptr> _master_pool;
    std::multimap<unsigned, conn_ptr> _slave_pool;
    std::mutex _pools_sync_mux;
    void __p_insert(const std::string & ip_,
                    unsigned port_,
                    std::multimap<unsigned, conn_ptr> &target_,
                    unsigned priority_ = 100);

    conn_ptr __get_best_rand(std::multimap<unsigned, conn_ptr> &target_);
};

}
#endif // CONN_POOL_HPP
