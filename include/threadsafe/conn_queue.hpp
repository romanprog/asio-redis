#ifndef CONN_QUEUE_HPP
#define CONN_QUEUE_HPP

#include "queue.hpp"

namespace redis {

namespace threadsafe {

template <typename AllocT, typename LockT = std::mutex>
class conn_queue : public queue<AllocT, LockT>
{
public:
    conn_queue() = default;

    bool get_first_free(AllocT & result_)
    {
        std::lock_guard<LockT> lm(this->_rw_lock);

        size_t sz = this->_base_queue.size();

        for (size_t i = 0; i < sz; ++i)
        {
            this->_base_queue.push(std::move(this->_base_queue.front()));
            this->_base_queue.pop();

            if (this->_base_queue.back().use_count() == 1)
            {
                result_ = this->_base_queue.back();
                return true;
            }

        }
        return false;
    }

};


} // namespace ts

} // namespace redis

#endif // CONN_QUEUE_HPP
