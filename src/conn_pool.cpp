#include "../include/conn_pool.hpp"
#include "../include/thread_safe.hpp"
#include "../utils/h_net.hpp"


namespace redis {

conn::conn(const strand_ptr & ev, std::string ip_, unsigned port_)
    : _ev_loop(ev),
      _ip(std::move(ip_)),
      _port(port_)
{  }

void conn::get(get_soc_callback cb_)
{
    soc_ptr soc;
    asio::error_code ec;
    if (_cache.get_first_free(soc)) {
        cb_(ec, std::move(soc));
        return;
    }

    soc = std::make_shared<asio::ip::tcp::socket>(_ev_loop->get_io_service());
    asio::ip::address ip(asio::ip::address::from_string(_ip, ec));
    if (ec) {
        cb_(ec, nullptr);
        return;
    }

    auto connection_handler = [this, cb_, soc](asio::error_code ec) mutable
    {
        if (ec) {
            cb_(ec, nullptr);
            return;
        }
        cb_(ec, std::move(soc));
    };

    asio::ip::tcp::endpoint endpoint(ip, _port);
    soc->async_connect(endpoint, _ev_loop->wrap(connection_handler));

}


conn_pool::conn_pool(strand_ptr ev)
    :_ev_loop(ev)
{

}

void conn_pool::add_master(const std::string &ip_, unsigned port_, unsigned priority_)
{
    __p_insert(ip_, port_, _master_pool, priority_);
}

void conn_pool::add_slave(const std::string &ip_, unsigned port_, unsigned priority_)
{
    __p_insert(ip_, port_, _slave_pool, priority_);
}

void conn_pool::async_get_master(get_soc_callback cb_)
{
    conn_ptr best_rnd_cn = __get_best_rand(_master_pool);
    best_rnd_cn->get(cb_);
}

void conn_pool::async_get_slave(get_soc_callback cb_)
{
    conn_ptr best_rnd_cn = __get_best_rand(_slave_pool);
    best_rnd_cn->get(cb_);
}

void conn_pool::__p_insert(const std::string &ip_, unsigned port_, std::multimap<unsigned, conn_ptr> &target_, unsigned priority_)
{
    if (priority_ > 100 || priority_ < 1)
        throw std::logic_error("Priority value out of range (1-100).");

    if (!hnet::is_ip_v4(ip_))
        throw std::logic_error("Invalid ip");

    std::lock_guard<std::mutex> lock(_pools_sync_mux);
    target_.insert({priority_, std::make_shared<conn>(_ev_loop, ip_, port_)});
}

conn_ptr conn_pool::__get_best_rand(std::multimap<unsigned, conn_ptr> &target_)
{

    conn_ptr best_res;
    unsigned best_score {0};
    srand(time(NULL));

    std::lock_guard<std::mutex> lock(_pools_sync_mux);
    if (_master_pool.empty())
        throw std::logic_error("Spool is empty.");

    for (auto rit = target_.rbegin(); rit != target_.rend(); ++rit)
    {
        unsigned bst_rnd = rand() % rit->first;
        if (bst_rnd > best_score) {
            best_score = bst_rnd;
            best_res = rit->second;
        }
    }

    return best_res;
}



} //namespace redis
