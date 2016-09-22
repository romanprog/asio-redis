#ifndef SERIAL_HPP
#define SERIAL_HPP

#include "proc_abstract.hpp"

namespace redis {

namespace procs {

class serial : public proc_abstract
{
public:

    serial(strand_ptr main_loop_, soc_ptr && soc_, unsigned timeout_, disconection_cb dh_ = nullptr);
    ~serial();

    template <typename T, typename cbType, typename BuffType>
    void push(const query<T, BuffType> & q_, cbType && cb_)
    {
        if (_stop_in_progress) {
            resp_data resp;
            resp.sres = "Serials processors stoped. Query ignored.";
            resp.type = respond_type::error_str;
            cb_(100, resp);
            return;
        }

        // Waiting for free query space in buffer.
        // Lock mutex for for undivided operation sequence (callback push
        // to cb queue and add query to buffer).

        std::unique_lock<std::mutex> lock(_buff_mux);
        _sending_confirm_cond.wait(lock,
                                   [this](){ return _query_queue.size() <= max_buff_size; }
                    );
         _query_queue.push(serial_query_adapter(q_, cb_));
        lock.unlock();


         __req_proc_manager();
    }

private:

    const size_t max_query_in_multibuff {10};
    const size_t max_buff_size {100};
    std::mutex _buff_mux;
    threadsafe::queue<serial_query_adapter> _query_queue;
    threadsafe::queue<serial_query_adapter> _sended_queries;

    void __req_poc() override;
    void __resp_proc() override;
    bool queues_is_empty() override;
    bool nothing_to_send() override;

    void soc_error_callbacks() override;
};

using serial_ptr = std::shared_ptr<serial>;

} // namespace procs

} // namespace redis


#endif // SERIAL_HPP
