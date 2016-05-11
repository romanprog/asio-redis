#ifndef SERIAL_HPP
#define SERIAL_HPP

#include "../query.hpp"
#include "../proto.hpp"
#include "../threadsafe/conn_queue.hpp"
#include "../buffers/io_buffers.hpp"

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

using serial_ptr = std::shared_ptr<serial>;

} // namespace procs

} // namespace redis


#endif // SERIAL_HPP
