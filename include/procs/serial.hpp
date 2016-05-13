#ifndef SERIAL_HPP
#define SERIAL_HPP

#include "../query.hpp"
#include "../proto.hpp"
#include "../threadsafe/conn_queue.hpp"
#include "../threadsafe/tests_lab.hpp"
#include "../buffers/io_buffers.hpp"
#include "../../utils/profiler.hpp"

#include <mutex>
#include <thread>
#include <future>

namespace redis {

namespace procs {

class serial
{
public:

    serial(strand_ptr main_loop_, soc_ptr && soc_);
    ~serial() = default;
    template <typename T, typename BuffType>
    void push(const query<T, BuffType> & q_)
    {
        profiler::global().startpoint();
         _query_queue.push(serial_query_adapter(q_));
         __proc_manager();
    }

private:
    strand_ptr _ev_loop;
    soc_ptr _socket;
    resp_proto _resp_parser;
    input_buff & _reading_buff;
    redis::resp_data _respond;

    threadsafe::queue<serial_query_adapter> _query_queue;

    std::atomic<bool> _proc_running {false};


    void __req_poc();
    void __proc_manager();
    void __resp_proc();
};


class async_serial
{
public:

    async_serial(strand_ptr main_loop_, soc_ptr && soc_);
    ~async_serial() = default;

    template <typename T, typename BuffType>
    void push(const query<T, BuffType> & q_)
    {
        {
            std::lock_guard<std::mutex> qlock(_queue_mux);
            _query_queue.push(serial_query_adapter(q_));
            _cb_queue.push(q_.get_callback());
        }
         __proc_manager();
    }

private:
    strand_ptr _ev_loop;
    soc_ptr _socket;
    resp_proto _resp_parser;
    input_buff & _reading_buff;
    redis::resp_data _respond;

    threadsafe::queue<serial_query_adapter> _query_queue;
    threadsafe::lab::queue_fast<RedisCallback> _cb_queue;

    std::atomic<bool> _proc_running {false};
    std::mutex _queue_mux;


    void __req_poc();
    void __proc_manager();
    void __resp_proc();
};

using serial_ptr = std::shared_ptr<serial>;
using async_serial_ptr = std::shared_ptr<async_serial>;

} // namespace procs

} // namespace redis


#endif // SERIAL_HPP
