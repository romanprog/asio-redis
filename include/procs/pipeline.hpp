#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include "../query.hpp"
#include "../proto.hpp"
#include "../thread_safe.hpp"
#include "../buffers/io_buffers.hpp"

#include <mutex>
#include <thread>
#include <future>

namespace redis {

namespace procs {

class pipeline
{
public:

    pipeline(strand_ptr main_loop_, soc_ptr && soc_);
    ~pipeline() = default;

    void push(const std::string &query_, RedisCallback cb_);

private:
    strand_ptr _ev_loop;
    soc_ptr _socket;
    resp_proto _resp_parser;
    input_buff & _reading_buff;
    output_buff _sending_buff;
    redis::resp_data _respond;

    threadsafe::functors_queue<void (int, const resp_data &)> _cb_queue;

    std::mutex _send_buff_mux;
    std::atomic<bool> _req_proc_running {false};
    std::atomic<bool> _stop_in_progress {true};

    std::promise<void> _work_done_waiter;

    void stop();
    void work_done_report();

    void __req_poc();
    void __req_proc_manager();
    void __resp_proc();
};

} // namespace procs

} // namespace redis

#endif // PIPELINE_HPP
