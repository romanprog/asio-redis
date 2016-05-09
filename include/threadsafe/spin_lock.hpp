#ifndef SPIN_LOCK_HPP
#define SPIN_LOCK_HPP

#include <atomic>

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


} // namespace ts

} // namespace redis

#endif // SPIN_LOCK_HPP
