#include "../include/client.hpp"

#include <asio.hpp>

namespace redis {


client::client()
    : _ev_loop(std::make_shared<asio::strand>(_io)),
      _worked_time(_ev_loop->get_io_service())
{
    reset_timer();
}

client::client(strand_ptr strand_)
    : _ev_loop(strand_),
      _worked_time(_ev_loop->get_io_service())
{
    reset_timer();
}

void client::async_connect(const std::string &ip_, unsigned port_, confirm_cb cb_)
{

}


void client::async_connect(const std::vector<srv_endpoint> &master_pool_,
                     const std::vector<srv_endpoint> &slave_pool_, confirm_cb cb_)
{

    for (const srv_endpoint & ep : master_pool_)
        _master_pool.add_unit(std::make_shared<conn_manager>(_ev_loop, ep.ip, ep.port), ep.pref);


    for (const srv_endpoint & ep : slave_pool_)
        _slave_pool.add_unit(std::make_shared<conn_manager>(_ev_loop, ep.ip, ep.port), ep.pref);

    __ainit_mpipeline_pool(cb_);

}

std::future<asio::error_code> client::future_connect(const std::vector<srv_endpoint> &master_pool_,
                                                     const std::vector<srv_endpoint> &slave_pool_)
{
    std::shared_ptr<std::promise<asio::error_code>> prom_ptr = std::make_shared<std::promise<asio::error_code>>();

    auto all_connect_handler = [prom_ptr](asio::error_code ec) mutable
    {
        prom_ptr->set_value(ec);
    };

     async_connect(master_pool_, slave_pool_, all_connect_handler);

     return prom_ptr->get_future();

//    return;
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
    _thread_worker.join();
}

void client::__ainit_mpipeline_pool(confirm_cb cb_, unsigned unit_num)
{
    conn_manager_ptr cm_tmp = _master_pool.get_list()[unit_num].first;
    unsigned pref = _master_pool.get_list()[unit_num].second;

    cm_tmp->async_get([this, cb_, unit_num, pref](asio::error_code& ec, soc_ptr && result)
    {
        if (ec) {
            cb_(ec);
            return;
        }

        _master_pipeline.add_unit(std::make_shared<procs::pipeline>(_ev_loop, std::move(result)), pref);

        if (unit_num == _master_pool.size() - 1) {
            __ainit_spipeline_pool(cb_);
            return;
        }
        __ainit_mpipeline_pool(cb_, unit_num + 1);

    });
}

void client::__ainit_spipeline_pool(confirm_cb cb_, unsigned unit_num)
{
    conn_manager_ptr cm_tmp = _slave_pool.get_list()[unit_num].first;
    unsigned pref = _slave_pool.get_list()[unit_num].second;

    cm_tmp->async_get([this, cb_, unit_num, pref](asio::error_code& ec, soc_ptr && result)
    {
        if (ec) {
            cb_(ec);
            return;
        }

        _slave_pipeline.add_unit(std::make_shared<procs::pipeline>(_ev_loop, std::move(result)), pref);

        if (unit_num == _slave_pool.size() - 1) {
            __ainit_mserial_pool(cb_);
            return;
        }
        __ainit_spipeline_pool(cb_, unit_num + 1);

    });
}

void client::__ainit_mserial_pool(confirm_cb cb_, unsigned unit_num)
{
    conn_manager_ptr cm_tmp = _master_pool.get_list()[unit_num].first;
    unsigned pref = _master_pool.get_list()[unit_num].second;

    cm_tmp->async_get([this, cb_, unit_num, pref](asio::error_code& ec, soc_ptr && result)
    {
        if (ec) {
            cb_(ec);
            return;
        }

        _master_serial.add_unit(std::make_shared<procs::serial>(_ev_loop, std::move(result)), pref);

        if (unit_num == _master_pool.size() - 1) {
            __ainit_sserial_pool(cb_);
            return;
        }
        __ainit_mserial_pool(cb_, unit_num + 1);

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

        _slave_serial.add_unit(std::make_shared<procs::serial>(_ev_loop, std::move(result)), pref);

        if (unit_num == _slave_pool.size() - 1) {
            cb_(asio::error_code());
            return;
        }
        __ainit_sserial_pool(cb_, unit_num + 1);

    });
}



} //namespace redis
