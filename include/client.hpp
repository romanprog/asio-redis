#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <thread>
#include <asio/steady_timer.hpp>

#include "thread_safe.hpp"
#include "conn_pool.hpp"
#include "procs/pipeline.hpp"
#include "procs/serial.hpp"


namespace redis {

template <typename T>
using pool_ptr = std::shared_ptr<threadsafe::list_balancer<T>>;

using pipeline_pool = threadsafe::list_balancer_ext<procs::pipeline_ptr>;
using serial_pool = threadsafe::list_balancer_ext<procs::serial_ptr>;
using conn_mng_pool = threadsafe::list_balancer_ext<conn_manager_ptr>;

using get_conn_list_cb = std::function<void(asio::error_code,  std::vector<std::pair<soc_ptr, soc_ptr>> &&, unsigned )>;
using confirm_cb = std::function<void(asio::error_code)>;

class client
{
public:
    client();
    client(strand_ptr strand_);

    void async_connect(const std::string & ip_, unsigned port_, confirm_cb cb_);

    void async_connect(const std::vector<srv_endpoint> & master_pool_,
                       const std::vector<srv_endpoint> & slave_pool_,
                       confirm_cb cb_);
    void async_send(const std::string & query, RedisCallback cb_);
    template <typename CmdType>
    void async_send(const query<CmdType> & q_)
    {

    }

    std::future<asio::error_code> future_connect(const std::vector<srv_endpoint> & master_pool_,
                                                 const std::vector<srv_endpoint> & slave_pool_);
    void run_thread_worker();
    void reset_timer();

    ~client();

private:
    asio::io_service _io;
    strand_ptr _ev_loop;
    asio::steady_timer _worked_time;
    std::thread _thread_worker;

    conn_mng_pool _master_pool;
    conn_mng_pool _slave_pool;

    pipeline_pool _master_pipeline;
    pipeline_pool _slave_pipeline;

    serial_pool _master_serial;
    serial_pool _slave_serial;

    void __ainit_mpipeline_pool(confirm_cb cb_, unsigned unit_num = 0);
    void __ainit_spipeline_pool(confirm_cb cb_, unsigned unit_num = 0);
    void __ainit_mserial_pool(confirm_cb cb_, unsigned unit_num = 0);
    void __ainit_sserial_pool(confirm_cb cb_, unsigned unit_num = 0);


};

} // namespace redis
#endif // CLIENT_HPP
