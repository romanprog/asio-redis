#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include "proc_abstract.hpp"

#include <mutex>
#include <thread>
#include <future>

namespace redis {

namespace procs {

class pipeline : public proc_abstract
{
public:

    pipeline(strand_ptr main_loop_, soc_ptr && soc_, unsigned timeout_ = 3);
    ~pipeline();

    void push(redis_callback cb_, const std::string &query_, bool one_line_query = false);
    void set_timeout(unsigned timeout_);

private:

    threadsafe::queue<redis_callback> _cb_queue;
    std::mutex _send_buff_mux;

    void __req_poc() override;
    void __resp_proc() override;
    inline bool queues_is_empty() override;
    inline bool nothing_to_send() override;
};

using pipeline_ptr = std::shared_ptr<pipeline>;

} // namespace procs

} // namespace redis

#endif // PIPELINE_HPP
