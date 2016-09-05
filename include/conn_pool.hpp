#ifndef CONN_POOL_HPP
#define CONN_POOL_HPP
#include "types.hpp"
#include "threadsafe/conn_queue.hpp"
#include "unordered_map"

#include <string>

namespace redis {

class conn_manager
{
public:
    conn_manager(const strand_ptr & ev, std::string ip_, unsigned port_);
    void async_get(get_soc_callback cb_, unsigned timeout_ = 3);
    soc_ptr get(asio::error_code &err, unsigned timeout_ = 3);
private:
    strand_ptr _ev_loop;
    threadsafe::conn_queue _cache;
    std::string _ip;
    unsigned _port;
};

using conn_manager_ptr = std::shared_ptr<conn_manager>;

}
#endif // CONN_POOL_HPP
