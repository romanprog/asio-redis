#include "../include/procs/pipeline.hpp"

#include <asio.hpp>

namespace redis {

namespace procs {


pipeline::pipeline(strand_ptr main_loop_, soc_ptr &&soc_, unsigned timeout_)
    : _ev_loop(std::move(main_loop_)),
      _socket(std::move(soc_)),
      _reading_buff(_resp_parser.buff()),
      _timeout_clock(_ev_loop->get_io_service()),
      _timeout_seconds(timeout_)
{
    __resp_proc();
}

pipeline::~pipeline()
{
    stop();
    _socket->cancel();
    _socket->close();
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

    {
        std::lock_guard<std::mutex> lock(_send_buff_mux);

        _cb_queue.push(cb_);
        _sending_buff.add_query(query_, one_line_query);
    }
    __req_proc_manager();
}

void pipeline::set_timeout(unsigned timeout_)
{
    if (timeout_)
        _timeout_seconds = timeout_;
    else
        throw std::logic_error("Can not set 0 seconds timeout.");
}

void pipeline::stop()
{
    _stop_in_progress = true;
    if (!_cb_queue.empty()) {
        auto _local_waiter = _work_done_waiter.get_future().share();
        _local_waiter.wait();
    }
}

void pipeline::work_done_report()
{
    _work_done_waiter.set_value();
}

void pipeline::__socket_error_hendler(std::error_code ec)
{

    throw std::logic_error(ec.message());
}

void pipeline::__timeout_hendler()
{
    throw std::logic_error("Socket timeout!");
}

void pipeline::__reset_timeout()
{
    _timeout_clock.expires_from_now(std::chrono::seconds(_timeout_seconds));
    _timeout_clock.async_wait([this](asio::error_code ec)
    {
        if (!ec)
            __timeout_hendler();
    });
}


void pipeline::__req_poc()
{
    auto req_handler = [this](std::error_code ec, std::size_t bytes_sent)
    {
        if (!ec) {
            __reset_timeout();
            std::lock_guard<std::mutex> lock(_send_buff_mux);
            _sending_buff.sending_report(bytes_sent);
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

    _socket->async_write_some(asio::buffer(_sending_buff.new_data(), _sending_buff.new_data_size()), _ev_loop->wrap(req_handler));
}

void pipeline::__req_proc_manager()
{
    bool cmp_tmp {false};

    if (_req_proc_running.compare_exchange_strong(cmp_tmp, true))
    {
        if (_sending_buff.nothing_to_send()) {
            _req_proc_running.store(false);
            return;
        }
        __req_poc();
    }
}

void pipeline::__resp_proc()
{
    _reading_buff.release(2048);
    auto resp_handler = [this](std::error_code ec, std::size_t bytes_sent)
    {
          if (ec) {
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

} // namespace procs

} // namespace redis
