#ifndef CONN_POOL_HPP
#define CONN_POOL_HPP
#include "types.hpp"
#include "threadsafe/conn_queue.hpp"
#include "unordered_map"

#include <asio.hpp>
#include <string>

namespace redis {

class conn_manager
{
public:
    conn_manager(const strand_ptr & ev, std::string ip_, unsigned port_);
    void async_get(get_soc_callback cb_, unsigned timeout_ = 5);
//    void async_get(get_socs_list_callback cb_, unsigned count_, unsigned timeout_ = 5);
    soc_ptr get(asio::error_code &err, unsigned timeout_ = 5);
private:
    strand_ptr _ev_loop;
    threadsafe::conn_queue _cache;
    std::string _ip;
    unsigned _port;
//    void __async_get_one(std::vector<soc_ptr> &&result, get_socs_list_callback cb_, unsigned count_, unsigned timeout_ = 1);
};

using conn_manager_ptr = std::shared_ptr<conn_manager>;
//typedef threadsafe::ts_pool<conn_endpoint> master_pool;
//typedef threadsafe::ts_pool<conn_endpoint> slave_pool;

}
#endif // CONN_POOL_HPP
