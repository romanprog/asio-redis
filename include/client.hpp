#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <thread>
#include <asio/steady_timer.hpp>

#include "threadsafe/conn_queue.hpp"
#include "conn_pool.hpp"
#include "procs/pipeline.hpp"
#include "procs/serial.hpp"


namespace redis {

using pipeline_pool = threadsafe::list_balancer_ext<procs::pipeline_ptr>;
using serial_pool = threadsafe::list_balancer_ext<procs::serial_ptr>;
using conn_mng_pool = threadsafe::list_balancer_ext<conn_manager_ptr>;
using confirm_cb = std::function<void(asio::error_code)>;

class client
{
public:
    client();
    client(strand_ptr strand_);

    /// //////////////////////////// Connection section //////////////////////////////////////////////
    ///
    /// //////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief async_connect. "Master only" owerload.
    /// Asynchronous connects to master server.
    /// All queryes to this variant of client will be sent to master server.
    /// \param master_ip_. String IPv4 of remote redis server.
    /// \param master_port_. Remote redis server port nubber.
    /// \param cb_. Confirmation callback. Return standart asio::error_code.
    /// Function return immediately.
    /// //////////////////////////////////////////////////////////////////////////////////////////////
    void async_connect(const std::string & master_ip_, unsigned master_port_, confirm_cb cb_);

    /// //////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief future_connect. "Master only" owerload with future result.
    /// Future variant of previous async_connect.
    /// \param master_ip_. String IPv4 of remote redis server.
    /// \param master_port_. Remote redis server port nubber.
    /// \return std::future<asio::error_code>. After future_connect() use - future wait
    /// method (of returned future) blocks execution until all connections have been established
    /// or an error occurs.
    /// Function return immediately.
    /// Future @get() method return asio::error_code as connection resut.
    /// ///////////////////////////////////////////////////////////////////////////////////////////////
    std::future<asio::error_code> future_connect(const std::string & master_ip_, unsigned master_port_);

    /// //////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief async_connect. "Slaves pool only" owerload.
    /// Asynchronous connects to pool of slave redis servers. Establishes two connections to each.
    /// Request that can be executed only on the master server cause an exception of the client
    /// connected in this way..
    /// \param slave_pool_. List of srv_endpoint which include ip, port and preference of slave server (1-10).
    /// \param cb_. Confirmation callback. Return standart asio::error_code.
    /// //////////////////////////////////////////////////////////////////////////////////////////////
    void async_connect(const std::vector<srv_endpoint> & slave_pool_, confirm_cb cb_);

    /// //////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief future_connect. "Slaves pool only" owerload with future result.
    /// Future variant of previous async_connect.
    /// \param slave_pool_. List of srv_endpoint which include ip, port and preference of slave server (1-10)
    /// \return std::future<asio::error_code>. After future_connect() use - future wait
    /// method (of returned future) blocks execution until all connections have been
    /// established or an error occurs.
    /// Function return immediately.Confirmation callback. Return standart asio::error_code.
    /// Future @get() method return asio::error_code as connection resut.
    /// //////////////////////////////////////////////////////////////////////////////////////////////
    std::future<asio::error_code> future_connect(const std::vector<srv_endpoint> & slave_pool_);

    /// //////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief async_connect. "Master - slaves pool" owerload.
    /// Asynchronous connects to master redis server and pool of it slaves. Establishes two
    /// connections to each.
    /// \param master_ip_. String IPv4 of remote redis server.
    /// \param master_port_. Remote redis server port nubber.
    /// \param slave_pool_. List of srv_endpoint which include ip, port and preference of slave server (1-10)
    /// Preference used for balancing load on slaves pool.
    /// \param cb_. Confirmation callback. Return standart asio::error_code.
    /// //////////////////////////////////////////////////////////////////////////////////////////////
    void async_connect(const std::string & master_ip_,
                       unsigned master_port_,
                       const std::vector<srv_endpoint> & slave_pool_,
                       confirm_cb cb_);

    /// //////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief future_connect. "Master - slaves pool" owerload with future result.
    /// Future variant of previous async_connect.
    /// \param master_ip_. String format IPv4 of remote redis server.
    /// \param master_port_. Remote redis server port num.
    /// \param slave_pool_. List of srv_endpoint which include ip, port and preference of slave server (1-10)
    /// \return std::future<asio::error_code>. After future_connect() use - future wait
    /// method (of returned future) blocks execution until all connections have been
    /// established or an error occurs.
    /// Function return immediately.
    /// Future @get() method return asio::error_code as connection resut.
    /// ///////////////////////////////////////////////////////////////////////////////////////////////
    std::future<asio::error_code> future_connect(const std::string & master_ip_,
                                                 unsigned master_port_,
                                                 const std::vector<srv_endpoint> & slave_pool_);



    // /////////////////// Queryes section //////////////////////////////////////////////////

    std::future<resp_data> future_send(const std::string & query);

    void async_send(const std::string & query, RedisCallback cb_);


    template <typename CmdType, typename cbType>
    void async_send(const query<CmdType, buff::common_buffer> & q_, cbType && cb_)
    {
        async_send_pipe(q_, std::forward<cbType>(cb_), typename CmdType::only_master_t());
    }

    template <typename CmdType, typename cbType>
    void async_send(const query<CmdType, buff::direct_write_buffer> & q_, cbType && cb_)
    {
        async_send_serial(q_, std::forward<cbType>(cb_), typename CmdType::only_master_t());
    }

    template <typename CmdType, typename cbType>
    void async_send_master(const query<CmdType, buff::common_buffer> & q_, cbType && cb_)
    {
        async_send_pipe(q_, std::forward<cbType>(cb_), std::true_type());
    }

    template <typename CmdType, typename cbType>
    void async_send_master(const query<CmdType, buff::direct_write_buffer> & q_, cbType && cb_)
    {
        async_send_serial(q_, std::forward<cbType>(cb_),  std::true_type());
    }

    void run_thread_worker();


    ~client();

private:
    asio::io_service _io;
    strand_ptr _ev_loop;
    asio::steady_timer _worked_time;
    std::thread _thread_worker;

    conn_manager_ptr _master_conn;
    procs::pipeline_ptr _master_pipeline;
    procs::serial_ptr _master_serial;

    conn_mng_pool _slave_pool;
    pipeline_pool _slave_pipeline_pool;
    serial_pool _slave_serial_pool;

    void reset_timer();

    void __init_master_pipeline(confirm_cb cb_);
    void __init_master_serial(confirm_cb cb_);
    void __ainit_spipeline_pool(confirm_cb cb_, unsigned unit_num = 0);
    void __ainit_sserial_pool(confirm_cb cb_, unsigned unit_num = 0);

    template <typename CmdType, typename cbType>
    void async_send_pipe(const query<CmdType> & q_, cbType && cb_, std::true_type)
    {
        if (!_master_pipeline)
            throw std::logic_error("Pipeline: no valid connections.");

        _master_pipeline->push(std::forward<cbType>(cb_), q_.as_string_ref());
    }

    template <typename CmdType, typename cbType>
    void async_send_pipe(const query<CmdType> & q_, cbType && cb_, std::false_type)
    {
        if (_slave_pipeline_pool.empty())
            async_send_pipe(q_, std::forward<cbType>(cb_), std::true_type());
        else
            _slave_pipeline_pool.balanced_rand()->push(std::forward<cbType>(cb_), q_.as_string_ref());
    }

    template <typename CmdType, typename cbType, typename BType>
    void async_send_serial(const query<CmdType, BType> & q_, cbType && cb_, std::true_type)
    {
        if (!_master_serial)
            throw std::logic_error("Serial: no valid connections.");

        _master_serial->push(q_, std::forward<cbType>(cb_));
    }

    template <typename CmdType, typename cbType, typename BType>
    void async_send_serial(const query<CmdType, BType> & q_, cbType && cb_, std::false_type)
    {
        if (_slave_serial_pool.empty())
            async_send_serial(q_, std::forward<cbType>(cb_), std::true_type());
        else
            _slave_serial_pool.balanced_rand()->push(q_, std::forward<cbType>(cb_));
    }


};

} // namespace redis
#endif // CLIENT_HPP
