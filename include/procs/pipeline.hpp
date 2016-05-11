#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include "../query.hpp"
#include "../proto.hpp"
#include "../threadsafe/threadsafe.hpp"
#include "../threadsafe/tests_lab.hpp"
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
    ~pipeline();

    void push(RedisCallback cb_, const std::string &query_);

private:
    strand_ptr _ev_loop;
    soc_ptr _socket;
    resp_proto _resp_parser;
    input_buff & _reading_buff;
    output_buff _sending_buff;
    redis::resp_data _respond;

    // threadsafe::functors_queue<void (int, const resp_data &)> _cb_queue;
    threadsafe::lab::queue_fast<RedisCallback> _cb_queue;
    std::mutex _send_buff_mux;
    std::atomic<bool> _req_proc_running {false};
    std::atomic<bool> _stop_in_progress {false};

    std::promise<void> _work_done_waiter;

    void stop();
    void work_done_report();

    void __req_poc();
    void __req_proc_manager();
    void __resp_proc();
};

using pipeline_ptr = std::shared_ptr<pipeline>;


class pipeline_new
{
public:

    pipeline_new(strand_ptr main_loop_, soc_ptr && soc_);
    ~pipeline_new();

    void push(std::shared_ptr<std::string> query_, RedisCallback cb_);

private:
    strand_ptr _ev_loop;
    soc_ptr _socket;
    resp_proto _resp_parser;
    input_buff & _reading_buff;
    redis::resp_data _respond;


    using string_ptr = std::shared_ptr<std::string>;

    // threadsafe::functors_queue<void (int, const resp_data &)> _cb_queue;
    threadsafe::lab::queue_fast<string_ptr> _query_queue;
    threadsafe::lab::queue_fast<RedisCallback> _cb_queue;


    std::vector<string_ptr> _data_ptr_list;
    std::vector<asio::const_buffer> _sending_multibuff;


    std::mutex _send_buff_mux;
    std::atomic<bool> _req_proc_running {false};
    std::atomic<bool> _stop_in_progress {false};

    std::promise<void> _work_done_waiter;

    void stop();
    void work_done_report();

    void __req_poc();
    void __req_proc_manager();
    void __resp_proc();

    unsigned __build_multibuf(unsigned max_count = 100);
};

using pipeline_new_ptr = std::shared_ptr<pipeline_new>;

} // namespace procs

} // namespace redis

#endif // PIPELINE_HPP
