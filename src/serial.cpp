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

serial::~serial()
{
    stop();
    _socket->cancel();
    _socket->close();
}

void serial::stop()
{
    _stop_in_progress = true;
    if (!_sended_queries.empty() || !_query_queue.empty()) {
        auto _local_waiter = _work_done_waiter.get_future().share();
        _local_waiter.wait();
    }
}

void serial::work_done_report()
{
    _work_done_waiter.set_value();
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
                _proc_running.store(false);
                __proc_manager();
            }
        }
        //profiler::global().checkpoint("req");
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
    asio::async_write(*_socket, multibuf_tmp, _ev_loop->wrap(req_handler));
}

void serial::__proc_manager()
{
    bool cmp_tmp {false};

    if (_proc_running.compare_exchange_strong(cmp_tmp, true))
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

          //profiler::global().checkpoint("resp");


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
              _proc_running.store(false);
              __proc_manager();
              return;
          }

          __resp_proc();
    };

    _socket->async_read_some(asio::buffer(_reading_buff.data_top(), _reading_buff.size_avail()), _ev_loop->wrap(resp_handler));
}

} // namespace procs

} // namespace redis
