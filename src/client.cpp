#include "../include/client.hpp"

#include <asio.hpp>

namespace redis {


client::client()
    : _ev_loop(std::make_shared<asio::strand>(_io)),
      _worked_time(_ev_loop->get_io_service())
{
    reset_timer();
    run_thread_worker();
}

client::client(strand_ptr strand_)
    : _ev_loop(strand_),
      _worked_time(_ev_loop->get_io_service())
{
    reset_timer();
}

void client::async_connect(const std::string &master_ip_, unsigned master_port_, confirm_cb cb_)
{
    _master_conn = std::make_shared<conn_manager>(_ev_loop, master_ip_, master_port_);
    __init_master_pipeline(cb_);
}

std::future<asio::error_code> client::future_connect(const std::string &master_ip_, unsigned master_port_)
{
    std::shared_ptr<std::promise<asio::error_code>> prom_ptr = std::make_shared<std::promise<asio::error_code>>();
    auto all_connect_handler = [prom_ptr](asio::error_code ec) mutable
    {
        prom_ptr->set_value(ec);
    };

    async_connect(master_ip_, master_port_, all_connect_handler);
    return prom_ptr->get_future();
}

void client::async_connect(const std::vector<srv_endpoint> &slave_pool_, confirm_cb cb_)
{
    for (const srv_endpoint & ep : slave_pool_)
        _slave_pool.add_unit(std::make_shared<conn_manager>(_ev_loop, ep.ip, ep.port), ep.pref);

    __ainit_spipeline_pool(cb_);
}

std::future<asio::error_code> client::future_connect(const std::vector<srv_endpoint> &slave_pool_)
{
    std::shared_ptr<std::promise<asio::error_code>> prom_ptr = std::make_shared<std::promise<asio::error_code>>();
    auto all_connect_handler = [prom_ptr](asio::error_code ec) mutable
    {
        prom_ptr->set_value(ec);
    };

    async_connect(slave_pool_, all_connect_handler);
    return prom_ptr->get_future();
}


void client::async_connect(const std::string &master_ip_, unsigned master_port_,
                           const std::vector<srv_endpoint> &slave_pool_, confirm_cb cb_)
{

    _master_conn = std::make_shared<conn_manager>(_ev_loop, master_ip_, master_port_);

    for (const srv_endpoint & ep : slave_pool_)
        _slave_pool.add_unit(std::make_shared<conn_manager>(_ev_loop, ep.ip, ep.port), ep.pref);

    __init_master_pipeline(cb_);

}

void client::async_send(const std::string &query, RedisCallback cb_)
{
    _master_pipeline->push(cb_, query, true);
}

std::future<asio::error_code> client::future_connect(const std::string &master_ip_, unsigned master_port_,
                                                     const std::vector<srv_endpoint> &slave_pool_)
{
    std::shared_ptr<std::promise<asio::error_code>> prom_ptr = std::make_shared<std::promise<asio::error_code>>();
    auto all_connect_handler = [prom_ptr](asio::error_code ec) mutable
    {
        prom_ptr->set_value(ec);
    };

    async_connect(master_ip_, master_port_, slave_pool_, all_connect_handler);
    return prom_ptr->get_future();
}


void client::run_thread_worker()
{
    _thread_worker = std::thread(
                [this]()
    {
        _ev_loop->get_io_service().run();
    });
}

void client::reset_timer()
{
    _worked_time.expires_from_now(std::chrono::seconds(5));
    _worked_time.async_wait([this](asio::error_code ec)
    {
        if (!ec)
            reset_timer();
    });
}

client::~client()
{
    _worked_time.cancel();

    _master_pipeline.reset();
    _master_serial.reset();

    _slave_pipeline_pool.clear();
    _slave_serial_pool.clear();

    _thread_worker.join();
}

void client::__init_master_pipeline(confirm_cb cb_)
{
    if (!_master_conn) {
        __ainit_spipeline_pool(cb_);
        return;
    }

    _master_conn->async_get([this, cb_](asio::error_code& ec, soc_ptr && result)
    {
        if (ec) {
            cb_(ec);
            return;
        }
        _master_pipeline = std::make_shared<procs::pipeline>(_ev_loop, std::move(result));
        __init_master_serial(cb_);
    });
}

void client::__init_master_serial(confirm_cb cb_)
{
    _master_conn->async_get([this, cb_](asio::error_code& ec, soc_ptr && result)
    {
        if (ec) {
            cb_(ec);
            return;
        }
        _master_serial = std::make_shared<procs::serial>(_ev_loop, std::move(result));
        __ainit_spipeline_pool(cb_);
    });
}

void client::__ainit_spipeline_pool(confirm_cb cb_, unsigned unit_num)
{

    if (_slave_pool.empty()) {
        cb_(asio::error_code());
        return;
    }

    conn_manager_ptr cm_tmp = _slave_pool.get_list()[unit_num].first;
    unsigned pref = _slave_pool.get_list()[unit_num].second;

    cm_tmp->async_get([this, cb_, unit_num, pref](asio::error_code& ec, soc_ptr && result)
    {
        if (ec) {
            cb_(ec);
            return;
        }

        _slave_pipeline_pool.add_unit(std::make_shared<procs::pipeline>(_ev_loop, std::move(result)), pref);

        if (unit_num == _slave_pool.size() - 1) {
            __ainit_sserial_pool(cb_);
            return;
        }
        __ainit_spipeline_pool(cb_, unit_num + 1);

    });
}


void client::__ainit_sserial_pool(confirm_cb cb_, unsigned unit_num)
{
    conn_manager_ptr cm_tmp = _slave_pool.get_list()[unit_num].first;
    unsigned pref = _slave_pool.get_list()[unit_num].second;

    cm_tmp->async_get([this, cb_, unit_num, pref](asio::error_code& ec, soc_ptr && result)
    {
        if (ec) {
            cb_(ec);
            return;
        }

        _slave_serial_pool.add_unit(std::make_shared<procs::serial>(_ev_loop, std::move(result)), pref);

        if (unit_num == _slave_pool.size() - 1) {
            cb_(asio::error_code());
            return;
        }
        __ainit_sserial_pool(cb_, unit_num + 1);
    });
}


} //namespace redis
