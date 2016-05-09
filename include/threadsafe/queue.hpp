#ifndef QUEUE_HPP
#define QUEUE_HPP

#include <memory>
#include <functional>
#include <mutex>
#include <queue>
#include <atomic>

namespace redis {
namespace threadsafe {

template <typename AllocT, typename LockT = std::mutex>
class queue
{
public:
    queue() = default;

    void push(AllocT val)
    {
        std::lock_guard<LockT> lm(_rw_lock);
        _base_queue.push(val);
    }

    const AllocT & front()
    {
        std::lock_guard<LockT> lm(_rw_lock);
        return _base_queue.front();
    }

    void pop()
    {
        std::lock_guard<LockT> lm(_rw_lock);

        if (_base_queue.empty())
            return;

        _base_queue.pop();
    }

    bool get_and_pop(AllocT & result_)
    {
        std::lock_guard<LockT> lm(_rw_lock);
        if (_base_queue.empty())
            return false;

        result_ = std::move(_base_queue.front());
        _base_queue.pop();
        return true;
    }

    bool get(AllocT & result_) const
    {
        std::lock_guard<LockT> lm(_rw_lock);
        if (_base_queue.empty())
            return false;

        result_ = _base_queue.front();
        return true;
    }

    void clear()
    {
        std::lock_guard<LockT> lm(_rw_lock);

        while (!_base_queue.empty())
            _base_queue.pop();

    }
    bool empty() const
    {
        std::lock_guard<LockT> lm(_rw_lock);
        return _base_queue.empty();
    }

protected:


    mutable LockT _rw_lock;
    std::queue<AllocT> _base_queue;

};

} // namespace ts
} // namespace redis


#endif // QUEUE_HPP
