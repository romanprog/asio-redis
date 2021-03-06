#include "../include/client.hpp"

#include <asio.hpp>
#include <functional>

namespace redis {


client::client(disconection_cb dh_)
    : _ev_loop(std::make_shared<asio::strand>(_io)),
      _worked_time(_ev_loop->get_io_service())
{
    reset_timer();
    run_thread_worker();
}

client::client(strand_ptr strand_, disconection_cb dh_)
    : _ev_loop(strand_),
      _worked_time(_ev_loop->get_io_service())
{
    reset_timer();
}

client::client(const cl_options &opts_, disconection_cb dh_)
    : _ev_loop(std::make_shared<asio::strand>(_io)),
      _worked_time(_ev_loop->get_io_service()),
      _opts(opts_)
{
    reset_timer();
    run_thread_worker();
}

client::client(strand_ptr strand_, const cl_options &opts_, disconection_cb dh_)
    : _ev_loop(strand_),
      _worked_time(_ev_loop->get_io_service()),
      _opts(opts_)
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

void client::async_send(const std::string &query, redis_callback cb_)
{
    if (!_connected.load())
    {
        resp_data err_respond;
        err_respond.type = respond_type::error_str;
        err_respond.sres = "Client disconnected";
        // Todo: add errors numbers and it descriptions.
        cb_(110, err_respond);
        return;
    }
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

void client::disconnect()
{
    _connected.store(false);
    _master_pipeline.reset();
    _master_serial.reset();

    _slave_pipeline_pool.clear();
    _slave_serial_pool.clear();
}

std::future<asio::error_code> client::future_reconnect()
{
    std::shared_ptr<std::promise<asio::error_code>> prom_ptr = std::make_shared<std::promise<asio::error_code>>();
    auto connected_handler = [prom_ptr](asio::error_code ec) mutable
    {
        prom_ptr->set_value(ec);
    };

    async_reconnect(connected_handler);
    return prom_ptr->get_future();
}

void client::async_reconnect(confirm_cb cb_)
{
    disconnect();
    if (!_master_conn && !_slave_pool.size())
        throw std::logic_error("Can't reconect. Have no endpoint. Try connect first.");

    __init_master_pipeline(cb_);
}

void client::set_opts(const cl_options & opts_)
{
    _opts = opts_;
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
    disconnect();
    _thread_worker.join();
}

void client::__proc_disconnect_handler(asio::error_code ec)
{
    _connected.store(false);
    if (_disc_handler)
        _disc_handler(ec);
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
        _master_pipeline = std::make_shared<procs::pipeline>(_ev_loop, std::move(result),
                                                             _opts.resp_timeout,
                                                             std::bind(&client::__proc_disconnect_handler, this, std::placeholders::_1));
        __init_master_serial(cb_);
    }, _opts.conn_timeout);
}

void client::__init_master_serial(confirm_cb cb_)
{
    _master_conn->async_get([this, cb_](asio::error_code& ec, soc_ptr && result)
    {
        if (ec) {
            cb_(ec);
            return;
        }
        _master_serial = std::make_shared<procs::serial>(_ev_loop, std::move(result),
                                                         _opts.resp_timeout,
                                                         std::bind(&client::__proc_disconnect_handler, this, std::placeholders::_1));
        __ainit_spipeline_pool(cb_);
    }, _opts.conn_timeout);
}

void client::__ainit_spipeline_pool(confirm_cb cb_, unsigned unit_num)
{

    if (_slave_pool.empty()) {
        _connected.store(true);
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
        auto proc_tmp = std::make_shared<procs::pipeline>(_ev_loop,
                                                          std::move(result),
                                                          _opts.resp_timeout,
                                                          std::bind(&client::__proc_disconnect_handler, this, std::placeholders::_1));

        _slave_pipeline_pool.add_unit(std::move(proc_tmp), pref);

        if (unit_num == _slave_pool.size() - 1) {
            __ainit_sserial_pool(cb_);
            return;
        }
        __ainit_spipeline_pool(cb_, unit_num + 1);

    }, _opts.conn_timeout);
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

        auto proc_tmp = std::make_shared<procs::serial>(_ev_loop,
                                                          std::move(result),
                                                          _opts.resp_timeout,
                                                          std::bind(&client::__proc_disconnect_handler, this, std::placeholders::_1));

        _slave_serial_pool.add_unit(std::move(proc_tmp), pref);

        if (unit_num == _slave_pool.size() - 1) {
            _connected.store(true);
            cb_(asio::error_code());
            return;
        }
        __ainit_sserial_pool(cb_, unit_num + 1);
    }, _opts.conn_timeout);
}


} //namespace redis
