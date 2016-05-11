#ifndef TESTS_LAB_HPP
#define TESTS_LAB_HPP
#include <memory>
#include <functional>
#include <mutex>
#include <queue>
#include <atomic>

#include "../buffers/buff_abstract.hpp"
#include "spin_lock.hpp"

/// ////////////////////////////////////////////////////
/// Testing lab for threadsafe algoritms and containers.
/// Have own namespace "lab".
/// ////////////////////////////////////////////////////

namespace redis {
namespace threadsafe {
namespace lab {

template <typename T, typename HeadLockT = std::mutex, typename TailLockT = redis::threadsafe::spin_lock>
class queue_fast
{
public:
    queue_fast()
        : _head(new node),
          _tail(_head)
    { }

    ~queue_fast()
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


private:

    int _size{0};
    struct node
    {
        T data;
        node * next;

    };

    node * _head;
    node * _tail;
    mutable HeadLockT _head_mux;
    mutable TailLockT _tail_mux;

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

template <typename T>
class queue
{
public:
    queue()
        : _head(new node),
          _tail(_head.get())
    { }

    void push(T new_value)
    {
        std::shared_ptr<T> new_data = std::make_shared<T>(std::move(new_value));

        std::unique_ptr<node> p(new node);

        {
            std::lock_guard<std::mutex> lc(_tail_mux);
            _tail->data = std::move(new_data);
            node * new_tail = p.get();
            _tail->next = std::move(p);
            _tail = new_tail;
        }
    }

    std::shared_ptr<T> try_pop()
    {
        std::unique_ptr<node> old_head = try_pop_head();
        return old_head ? old_head->data : std::shared_ptr<T>();
    }


private:

    int d_count{0};
    struct node
    {
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;

    };

    std::unique_ptr<node> _head;
    node * _tail;
    std::mutex _head_mux;
    std::mutex _tail_mux;

    node * get_tail()
    {
        std::lock_guard<std::mutex> lc(_tail_mux);
        return _tail;
    }

    std::unique_ptr<node> pop_head()
    {
        std::unique_ptr<node> old_head = std::move(_head);
        _head = std::move(old_head->next);
        return old_head;
    }

    std::unique_ptr<node> try_pop_head()
    {
        std::lock_guard<std::mutex> lc(_head_mux);
        if (_head.get() == get_tail())
            return std::unique_ptr<node>();

        return pop_head();
    }


};

class output_buff : public buff_abstract
{
public:
    output_buff();
    // Return true if all contained data already sended (and confirmed).
    bool nothing_to_send()
    {
        return top_offset() <= _sended_offset;
    }

    // Confirm data part (@bytes_sended size) sending.
    void sending_report(size_t bytes_sended)
    {
        _sended_offset += bytes_sended;

        if (nothing_to_send())
            manage_mem();
    }

    // Pointer to begining of new (not sended) data
    const char * new_data()
    {
            return data() + _sended_offset;
    }

    // Size of new (not sended) data.
    size_t new_data_size()
    {
            return top_offset() - _sended_offset;
    }

    // Add new query tu buffer line.
    void add_query(const std::string &query)
    {
        *this << query;
        *this << "\r\n";
    }

private:
    // Memory management: cleaning, fast reset, data transfer on free sites
    // to avoid the appearance of a new memory.
    void manage_mem();
    size_t _sended_offset {0};

};



} // namespace lab
} // namespace ts
} // namespace redis


#endif // TESTS_LAB_HPP
