#include "../include/procs/proc_abstract.hpp"
#include <iostream>

namespace redis {
namespace procs {

proc_abstract::proc_abstract(redis::strand_ptr main_loop_, redis::soc_ptr &&soc_, unsigned timeout_, disconection_cb dh_)
    : _ev_loop(std::move(main_loop_)),
      _socket(std::move(soc_)),
      _reading_buff(_resp_parser.buff()),
      _dsconn_handler(dh_),
      _timeout_clock(_ev_loop->get_io_service()),
      _timeout_seconds(timeout_)

{
}

proc_abstract::~proc_abstract()
{
    _socket->cancel();
    _socket->close();
}

void proc_abstract::__timeout_hendler()
{
    throw std::logic_error("Socket timeout!");
}

void proc_abstract::__reset_timeout()
{
    _timeout_clock.expires_from_now(std::chrono::milliseconds(_timeout_seconds));
    _timeout_clock.async_wait([this](asio::error_code ec)
    {
        if (!ec)
            __timeout_hendler();
    });
}

void proc_abstract::__req_proc_manager()
{
    bool cmp_tmp {false};

    if (_req_proc_running.compare_exchange_strong(cmp_tmp, true))
    {
        if (nothing_to_send()) {
            _req_proc_running.store(false);
            return;
        }
        __req_poc();
    }
}

void proc_abstract::stop()
{
    _stop_in_progress = true;
    if (!queues_is_empty()) {
        auto _local_waiter = _work_done_waiter.get_future().share();
        _local_waiter.wait();
    }
}

void proc_abstract::work_done_report()
{
    _work_done_waiter.set_value();
}

void proc_abstract::__socket_error_hendler(std::error_code ec)
{
    // Do this once.
    if (!_error_status) {
        _error_status = true;
        if (_dsconn_handler)
            _dsconn_handler(ec);
        std::cout << ec.message() << std::endl;
        // throw std::logic_error(ec.message());
    }
}

void proc_abstract::set_timeout(unsigned timeout_)
{
    if (timeout_)
        _timeout_seconds = timeout_;
    else
        throw std::logic_error("Can not set 0 seconds timeout.");
}

} // namespace redis
} // namespace procs
