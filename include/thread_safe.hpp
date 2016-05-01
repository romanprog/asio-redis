#ifndef THREAD_SAFE_HPP
#define THREAD_SAFE_HPP
#include <memory>
#include <functional>
#include <mutex>
#include <queue>
#include <atomic>
#include <iostream>

namespace redis {

namespace threadsafe {

class spin_lock
{
public:
    spin_lock()
        : _lock_flag(ATOMIC_FLAG_INIT)
    {}

    void lock()
    {
        while(_lock_flag.test_and_set(std::memory_order_acquire));
    }

    void unlock()
    {
        _lock_flag.clear(std::memory_order_release);
    }


private:
    std::atomic_flag _lock_flag;
};

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

    void dbg_print()
    {
        std::cout << this->_base_queue.front().use_count() << std::endl;
    }

};

} //namespace threadsafe

} //namespace redis
#endif // THREAD_SAFE_HPP
