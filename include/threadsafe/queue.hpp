#ifndef QUEUE_HPP
#define QUEUE_HPP

#include "spin_lock.hpp"

#include <memory>
#include <functional>
#include <mutex>
#include <queue>
#include <atomic>

namespace redis {
namespace threadsafe {

template <typename T, typename HeadLockT = std::mutex, typename TailLockT = redis::threadsafe::spin_lock>
class queue
{
public:
    queue()
        : _head(new node),
          _tail(_head)
    { }

    ~queue()
    {
        while (_head != _tail)
        {
            node * dl = _head;
            _head = dl->next;
            delete dl;
        }
        delete _head;
    }

    void push(T new_value)
    {
        node * p = new node;

        {
            std::lock_guard<TailLockT> lc(_tail_mux);
            _tail->data = std::move(new_value);
            _tail->next = p;
            _tail = p;
            ++_size;
        }
    }

    bool try_pop(T & res)
    {
        node * old_head = try_pop_head();

        if (!old_head)
            return false;

        res = std::move(old_head->data);
        delete old_head;
        return true;
    }

    bool empty() const
    {
        std::lock_guard<HeadLockT> lc(_head_mux);
        return !_size;
    }

    size_t size() const
    {
        return _size;
    }


private:

    std::atomic<size_t> _size{0};
    struct node
    {
        T data;
        node * next;

    };

    node * _head;
    node * _tail;
    mutable HeadLockT _head_mux;
    mutable TailLockT _tail_mux;

    friend class conn_queue;

    node * get_tail()
    {
        std::lock_guard<TailLockT> lc(_tail_mux);
        return _tail;
    }

    node * pop_head()
    {
        node * old_head = _head;
        _head = old_head->next;
        --_size;
        return old_head;
    }

    node * try_pop_head()
    {
        std::lock_guard<HeadLockT> lc(_head_mux);
        if (_head == get_tail())
            return nullptr;

        return pop_head();
    }


};

} // namespace ts
} // namespace redis


#endif // QUEUE_HPP
