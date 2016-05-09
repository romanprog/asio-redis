#include "../include/procs/pipeline.hpp"

#include <asio.hpp>

namespace redis {

namespace procs {


pipeline::pipeline(strand_ptr main_loop_, soc_ptr &&soc_)
    : _ev_loop(std::move(main_loop_)),
      _socket(std::move(soc_)),
      _reading_buff(_resp_parser.buff())
{
    __resp_proc();
}

pipeline::~pipeline()
{
    _socket->cancel();
    _socket->close();
}

void pipeline::push(const std::string &query_, RedisCallback cb_)
{

    if (_stop_in_progress) {
        resp_data resp;
        resp.sres = "Async processors stoped. Query ignored.";
        cb_(100, resp);
    }

    {
        std::lock_guard<std::mutex> lock(_send_buff_mux);
        _cb_queue.push(cb_);
        _sending_buff.add_query(query_);
    }
    __req_proc_manager();
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

void pipeline::__req_poc()
{
    auto req_handler = [this](std::error_code ec, std::size_t bytes_sent)
    {
        if (!ec) {
            std::lock_guard<std::mutex> lock(_send_buff_mux);
            _sending_buff.sending_report(bytes_sent);
        }

        _req_proc_running.store(false);
        __req_proc_manager();

    };
    _socket->async_write_some(asio::buffer(_sending_buff.new_data(), _sending_buff.new_data_size()), _ev_loop->wrap(req_handler));
}

void pipeline::__req_proc_manager()
{
    bool cmp_tmp {false};

    if (_req_proc_running.compare_exchange_strong(cmp_tmp, true, std::memory_order_release, std::memory_order_relaxed))
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
          if (ec)
              return;

          _reading_buff.accept(bytes_sent);

          while (_resp_parser.parse_one(_respond))
          {

              RedisCallback cb;
              // Call client function.
              if (!_cb_queue.try_pop(cb))
                  throw std::logic_error("No one callbacks(11). Query/resp processors sync error.");
              cb(1, _respond);

              if (_stop_in_progress && _cb_queue.empty())
                  work_done_report();

          }
          __resp_proc();
    };

    _socket->async_read_some(asio::buffer(_reading_buff.data_top(), _reading_buff.size_avail()), _ev_loop->wrap(resp_handler));
}

} // namespace procs

} // namespace redis
