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
    ~serial();

    template <typename T, typename cbType, typename BuffType,
              typename = std::enable_if_t<std::is_same<BuffType, buff::direct_read_buffer>::value
                                          >
              >
    void push(const query<T, BuffType> & q_, cbType && cb_)
    {
         _query_queue.push(serial_query_adapter(q_, cb_));
         __proc_manager();
    }

    template <typename T, typename cbType, typename BuffType>
    void push(const query<T, BuffType> & q_, cbType && cb_)
    {
         _query_queue.push(serial_query_adapter(q_, cb_));
         __proc_manager();
    }

private:
    strand_ptr _ev_loop;
    soc_ptr _socket;
    resp_proto _resp_parser;
    input_buff & _reading_buff;
    redis::resp_data _respond;
    size_t max_query_in_multibuff {10};

    threadsafe::queue<serial_query_adapter> _query_queue;
    threadsafe::queue<serial_query_adapter> _sended_queries;

    std::atomic<bool> _proc_running {false};


    void __req_poc();
    void __proc_manager();
    void __resp_proc();
};

using serial_ptr = std::shared_ptr<serial>;

} // namespace procs

} // namespace redis


#endif // SERIAL_HPP
