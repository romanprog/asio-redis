#ifndef SERIAL_HPP
#define SERIAL_HPP

#include "proc_abstract.hpp"

namespace redis {

namespace procs {

class serial : public proc_abstract
{
public:

    serial(strand_ptr main_loop_, soc_ptr && soc_, unsigned timeout_ = 3);
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

         _query_queue.push(serial_query_adapter(q_, cb_));
         __req_proc_manager();
    }

private:

    const size_t max_query_in_multibuff {10};
    threadsafe::queue<serial_query_adapter> _query_queue;
    threadsafe::queue<serial_query_adapter> _sended_queries;

    void __req_poc() override;
    void __resp_proc() override;
    inline bool queues_is_empty() override;
};

using serial_ptr = std::shared_ptr<serial>;

} // namespace procs

} // namespace redis


#endif // SERIAL_HPP
