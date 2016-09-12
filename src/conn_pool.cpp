#include "../include/conn_pool.hpp"
#include "../include/threadsafe/threadsafe.hpp"
#include "../include/utils/h_net.hpp"
#include "asio/steady_timer.hpp"
#include <thread>
#include <future>


namespace redis {

conn_manager::conn_manager(const strand_ptr & ev, std::string ip_, unsigned port_)
    : _ev_loop(ev),
      _ip(std::move(ip_)),
      _port(port_)
{  }

void conn_manager::async_get(get_soc_callback cb_, unsigned timeout_)
{
    soc_ptr soc_p;
    asio::error_code ec;
    if (_cache.get_first_free(soc_p)) {
        cb_(ec, std::move(soc_p));
        return;
    }

    soc_p = std::make_shared<asio::ip::tcp::socket>(_ev_loop->get_io_service());

    std::shared_ptr<asio::steady_timer> conn_timer = std::make_shared<asio::steady_timer>(_ev_loop->get_io_service());

    asio::ip::address ip(asio::ip::address::from_string(_ip, ec));
    if (ec) {
        cb_(ec, nullptr);
        return;
    }

    auto connection_handler = [this, cb_, soc_p, conn_timer](asio::error_code ec) mutable
    {
        if (ec) {
            cb_(ec, nullptr);
            conn_timer->cancel();
            return;
        }

        conn_timer->cancel();
        _cache.push(soc_p);
        cb_(ec, std::move(soc_p));
    };

    auto timeout_handler = [this, cb_, soc_p](asio::error_code ec) mutable
    {
        if (ec)
            return;

        ec.assign(asio::error::timed_out, asio::system_category());
        soc_p->cancel();
        cb_(ec, nullptr);
    };

    asio::ip::tcp::endpoint endpoint(ip, _port);
    soc_p->async_connect(endpoint, _ev_loop->wrap(connection_handler));
    conn_timer->expires_from_now(std::chrono::milliseconds(timeout_));
    conn_timer->async_wait(_ev_loop->wrap(timeout_handler));

}

soc_ptr conn_manager::get(asio::error_code & err, unsigned timeout_)
{
    std::promise<soc_ptr> conn_prom;
    std::future<soc_ptr> conn_fut = conn_prom.get_future();

    auto conn_handler = [&conn_prom, &err] (asio::error_code ec_, soc_ptr soc_) {
        err = ec_;
        conn_prom.set_value(std::move(soc_));
    };

    async_get(conn_handler, timeout_);
    conn_fut.wait();
    return conn_fut.get();
}

} //namespace redis
