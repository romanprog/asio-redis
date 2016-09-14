#include "../include/procs/pipeline.hpp"

#include <asio.hpp>
#include <utility>

namespace redis {

namespace procs {


pipeline::pipeline(strand_ptr main_loop_, soc_ptr &&soc_, unsigned timeout_)
    :proc_abstract::proc_abstract(std::move(main_loop_), std::move(soc_), timeout_)
{
    __resp_proc();
}

pipeline::~pipeline()
{
    stop();
}

void pipeline::push(redis_callback cb_, const std::string &query_, bool one_line_query)
{

    if (_stop_in_progress) {
        resp_data resp;
        resp.sres = "Pipeline processors stoped. Query ignored.";
        resp.type = respond_type::error_str;
        cb_(100, resp);
        return;
    }
    // Waiting for free query space in buffer.
    // Lock mutex for for undivided operation sequence (callback push
    // to cb queue and add query to buffer).
    std::unique_lock<std::mutex> lock(_buff_mux);
    _sending_confirm_cond.wait(lock,
                               [this, &query_](){ return !_sending_buff.check_overflow(query_.size()); }
                              );

    _cb_queue.push(cb_);
    _sending_buff.add_query(query_, one_line_query);
    lock.unlock();

    __req_proc_manager();
}



void pipeline::__req_poc()
{
    auto req_handler = [this](std::error_code ec, std::size_t bytes_sent)
    {
        _sending_buff.read_mem_locker()->unlock();

        if (!ec) {
            __reset_timeout();
            // Notify all clients, waiting for free space in buffer.
            // Lock mutex for sending "data transfer report" for buffer
            // (output_buff::sending_report modify buffer state).
            std::lock_guard<std::mutex> lock(_buff_mux);
            _sending_buff.sending_report(bytes_sent);
            _sending_confirm_cond.notify_all();
        } else
        {
            __socket_error_hendler(ec);
            return;
        }


        _req_proc_running.store(false);
        __req_proc_manager();

    };

    if (!_timer_is_started) {
        _timer_is_started = true;
        __reset_timeout();
    }
    _sending_buff.read_mem_locker()->lock();
    _socket->async_send(asio::buffer(_sending_buff.new_data(), _sending_buff.new_data_size()), _ev_loop->wrap(req_handler));

}

void pipeline::__resp_proc()
{
    _reading_buff.release(resp_release_sz);
    auto resp_handler = [this](std::error_code ec, std::size_t bytes_sent)
    {
          if (ec) {
              if (!_cb_queue.empty())
                __socket_error_hendler(ec);

              return;
          }

          _reading_buff.accept(bytes_sent);

          while (_resp_parser.parse_one(_respond))
          {
              redis_callback cb;
              // Call client function.
              if (!_cb_queue.try_pop(cb))
                  throw std::logic_error("No one callbacks(11). Query/resp processors sync error.");

              cb(0, _respond);

              if (_stop_in_progress && _cb_queue.empty())
              {
                  work_done_report();
                  return;
              }

          }

          if (_cb_queue.empty()) {
              _timer_is_started = false;
              _timeout_clock.cancel();
          } else {
              __reset_timeout();
          }
          __resp_proc();
    };

    _socket->async_read_some(asio::buffer(_reading_buff.data_top(), _reading_buff.size_avail()), _ev_loop->wrap(resp_handler));
}

bool pipeline::queues_is_empty()
{
    return _cb_queue.empty();
}

bool pipeline::nothing_to_send()
{
    return _sending_buff.nothing_to_send();
}

} // namespace procs

} // namespace redis
