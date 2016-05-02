#include "../include/procs/serial.hpp"

#include <asio.hpp>

namespace redis {

namespace procs {

serial::serial(strand_ptr main_loop_, soc_ptr &&soc_)
    : _ev_loop(main_loop_),
      _socket(std::move(soc_)),
      _reading_buff(_resp_parser.buff())

{

}

void serial::__req_poc()
{
    auto req_handler = [this](std::error_code ec, std::size_t bytes_sent)
    {
        if (ec) {
            resp_data ret;
            _query_queue.front().callback(1, ret);
            _query_queue.pop();
            _proc_running.store(false);
            __proc_manager();
        }
        __resp_proc();
   };

    _socket->async_write_some(_query_queue.front().get_multibuffer(), _ev_loop->wrap(req_handler));
}

void serial::__proc_manager()
{
    bool cmp_tmp {false};

    if (_proc_running.compare_exchange_strong(cmp_tmp, true, std::memory_order_release, std::memory_order_relaxed))
    {
        if (_query_queue.empty()) {
            _proc_running.store(false);
            return;
        }
        __req_poc();
    }
}

void serial::__resp_proc()
{
    _reading_buff.release(2048);
    auto resp_handler = [this](std::error_code ec, std::size_t bytes_sent)
    {
          if (ec)
              return;

          _reading_buff.accept(bytes_sent);


          if (!_resp_parser.parse_one(_respond))  {
              __resp_proc();
          } else   {
              // Critical error. Answer resived, but no one callback in queue.
              if (_query_queue.empty())
                  throw std::logic_error("No one callbacks(11). Query/resp processors sync error.");

              // Call client function.
              _query_queue.front().callback(0, _respond);
              _query_queue.pop();

          }
          __proc_manager();
    };

    _socket->async_read_some(asio::buffer(_reading_buff.data_top(), _reading_buff.size_avail()), _ev_loop->wrap(resp_handler));
}



} // namespace procs

} // namespace redis
