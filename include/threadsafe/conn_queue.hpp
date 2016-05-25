#ifndef CONN_QUEUE_HPP
#define CONN_QUEUE_HPP

#include "queue.hpp"
#include "../types.hpp"

namespace redis {

namespace threadsafe {

class conn_queue : public queue<soc_ptr, std::mutex, std::mutex>
{
public:
    conn_queue() = default;

    bool get_first_free(soc_ptr & result_)
    {
        // Full lock.
        std::lock_guard<std::mutex> lc_h(this->_head_mux);
        std::lock_guard<std::mutex> lc_t(this->_tail_mux);

        size_t sz = this->size();
        for (size_t i = 0; i < sz; ++i)
        {
            node * head_tmp = this->_head;
            if (head_tmp == this->_head)
                return false;

            // Save pointer to tail.
            node * tail_tmp = this->_tail;

            // New empty node for tail.
            node * new_node = new node;

            // Move front to tail.
            this->_tail->data = std::move(head_tmp->data);
            this->_tail->next = new_node;
            this->_tail = new_node;
            this->_head = head_tmp->next;

            // Delete old head (data moved to tail).
            delete head_tmp;

            // If usecount of ptr == 1 - this connection not used. Return it.
            // Shared_ptr copy incr usecount.
            if (tail_tmp->data.use_count() == 1) {
                result_ = tail_tmp->data;
                return true;
            }
        }

        return false;
    }

};


} // namespace ts

} // namespace redis

#endif // CONN_QUEUE_HPP
