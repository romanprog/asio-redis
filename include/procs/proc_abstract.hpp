#ifndef PROC_ABSTRACT_HPP
#define PROC_ABSTRACT_HPP

#include <asio/steady_timer.hpp>

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

class proc_abstract
{
public:
    proc_abstract(strand_ptr main_loop_, soc_ptr && soc_, unsigned timeout_, disconection_cb dh_ = nullptr);
    virtual ~proc_abstract();
    void set_timeout(unsigned timeout_);

protected:
    strand_ptr _ev_loop;
    soc_ptr _socket;
    resp_proto _resp_parser;
    input_buff & _reading_buff;
    output_buff _sending_buff;
    redis::resp_data _respond;
    disconection_cb _dsconn_handler;

    // Timer
    asio::steady_timer _timeout_clock;
    unsigned _timeout_seconds;
    bool _timer_is_started {false};

    // Waiting for free space in buffer.
    std::condition_variable _sending_confirm_cond;

    // 2 Kb
    const size_t resp_release_sz {2048};

    std::atomic<bool> _req_proc_running {false};
    std::atomic<bool> _stop_in_progress {false};

    std::promise<void> _work_done_waiter;

    void stop();
    void work_done_report();
    void __socket_error_hendler(std::error_code ec);
    bool _error_status {false};

    // Timer
    void __timeout_hendler();
    void __reset_timeout();

    virtual void __req_poc() = 0;
    void __req_proc_manager();
    virtual void __resp_proc() = 0;

    virtual bool queues_is_empty() = 0;
    virtual bool nothing_to_send() = 0;

};


} // namespace procs
} // namespace reris

#endif // PROC_ABSTRACT_HPP
