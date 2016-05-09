#ifndef FUNCTORS_QUEUE_HPP
#define FUNCTORS_QUEUE_HPP

#include "queue.hpp"

namespace redis {
namespace threadsafe {


template <typename T, typename LockT = std::mutex, typename FnType = std::function<T>>
class functors_queue : public queue<FnType, LockT>
{
public:
    functors_queue() = default;

    template <typename... _Args>
    bool call_and_pop_all(_Args&&... __args)
    {
        std::lock_guard<LockT> lm(this->_rw_lock);

        while (!this->_base_queue.empty()) {
            this->_base_queue.front()(std::forward<_Args>(__args)...);
            this->_base_queue.pop();
        }

        return true;
    }
    template <typename... _Args>
    bool call_and_pop(_Args&&... __args)
    {
        std::lock_guard<LockT> lm(this->_rw_lock);

        if (this->_base_queue.empty())
                return false;

        this->_base_queue.front()(std::forward<_Args>(__args)...);
        this->_base_queue.pop();

        return true;
    }

private:


};

} // namespace ts

} // namespace redis

#endif // FUNCTORS_QUEUE_HPP
