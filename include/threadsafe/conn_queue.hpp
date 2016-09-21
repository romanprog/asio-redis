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

    void head_to_tail()
    {
        if (this->_size < 2)
            return;

        this->_tail->data = std::move(this->_head->data);
        this->_tail->next = this->_head;
        this->_tail = this->_head;
        this->_head = this->_head->next;
        this->_tail->next = nullptr;
    }

    bool get_first_free(soc_ptr & result_)
    {
        // Full lock.
        std::lock_guard<std::mutex> lc_h(this->_head_mux);
        std::lock_guard<std::mutex> lc_t(this->_tail_mux);

        size_t sz = this->size();
        for (size_t i = 0; i < sz; ++i)
        {

            // If usecount of ptr == 1 - this connection not used. Return it.
            // Shared_ptr copy incr usecount.
            if (this->_head->data.use_count() == 1) {
                result_ = this->_head->data;
                head_to_tail();
                return true;
            }

            head_to_tail();

        }

        return false;
    }

    void clear_free()
    {
        std::lock_guard<std::mutex> lc_h(this->_head_mux);
        std::lock_guard<std::mutex> lc_t(this->_tail_mux);

        size_t sz = this->size();
        for (size_t i = 0; i < sz; ++i)
        {
            // If usecount of ptr == 1 - this connection not used. Return it.
            // Shared_ptr copy incr usecount.
            if (this->_head->data.use_count() == 1) {
                node * head_tmp = this->_head;
                this->_head = this->_head->next;
                delete head_tmp;
                --this->_size;
            } else {
                head_to_tail();
            }
        }
    }

};


} // namespace ts

} // namespace redis

#endif // CONN_QUEUE_HPP
