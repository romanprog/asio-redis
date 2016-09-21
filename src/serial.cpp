#include "../include/procs/serial.hpp"

#include <asio.hpp>

namespace redis {

namespace procs {

serial::serial(strand_ptr main_loop_, soc_ptr &&soc_, unsigned timeout_, disconection_cb dh_)
    :proc_abstract::proc_abstract(std::move(main_loop_), std::move(soc_), timeout_, dh_)
{ }

serial::~serial()
{
    stop();
}


void serial::__req_poc()
{
    //profiler::global().startpoint();
    auto req_handler = [this](std::error_code ec, std::size_t bytes_sent)
    {
        if (ec) {
            serial_query_adapter q_tmp;
            while (_sended_queries.try_pop(q_tmp)) {
                auto cb = q_tmp.get_callback();
                cb(1, resp_data());
                _req_proc_running.store(false);
                __req_proc_manager();
            }
            __socket_error_hendler(ec);
        }
        __reset_timeout();
        __resp_proc();
    };

    // Build multibuffer
    unsigned mq_counter {0};
    serial_query_adapter q_tmp;
    std::vector<asio::const_buffer> multibuf_tmp;

    while (true)
    {
        if (!_query_queue.try_pop(q_tmp))
            break;

        auto b_tmp = q_tmp.get_multibuffer();
        multibuf_tmp.insert(multibuf_tmp.end(), b_tmp.cbegin(), b_tmp.cend());
        _sended_queries.push(q_tmp);

        if (mq_counter >= max_query_in_multibuff)
            break;

        mq_counter ++;
    }
    {
        std::lock_guard<std::mutex> lock(_buff_mux);
        _sending_confirm_cond.notify_all();
    }
    // Start timer first time for timeout handling.
    if (!_timer_is_started) {
        _timer_is_started = true;
        __reset_timeout();
    }

    asio::async_write(*_socket, multibuf_tmp, _ev_loop->wrap(req_handler));
}

void serial::__resp_proc()
{
    _reading_buff.release(resp_release_sz);
    auto resp_handler = [this](std::error_code ec, std::size_t bytes_sent)
    {
        __reset_timeout();

        if (ec) {
            if (!_sended_queries.empty() || !_query_queue.empty())
                __socket_error_hendler(ec);

            return;
        }
        _reading_buff.accept(bytes_sent);

        while (_resp_parser.parse_one(_respond))  {
            serial_query_adapter cb_tmp;
            if (!_sended_queries.try_pop(cb_tmp))
                throw std::logic_error("No one callbacks(11). Query/resp processors sync error.");

            cb_tmp.get_callback()(0, _respond);
        }

        if (_sended_queries.empty()) {
            if (_stop_in_progress && _query_queue.empty())
            {
                work_done_report();
                return;
            }
            _req_proc_running.store(false);
            __req_proc_manager();
            return;
        }

        __resp_proc();
    };

    _socket->async_read_some(asio::buffer(_reading_buff.data_top(), _reading_buff.size_avail()), _ev_loop->wrap(resp_handler));
}

bool serial::queues_is_empty()
{
    return (_sended_queries.empty() && _query_queue.empty());
}

bool serial::nothing_to_send()
{
    return _query_queue.empty();
}

} // namespace procs

} // namespace redis
