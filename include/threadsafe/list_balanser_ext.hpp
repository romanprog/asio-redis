#ifndef LIST_BALANSER_EXT_HPP
#define LIST_BALANSER_EXT_HPP

#include "list_balanser.hpp"

namespace redis {

namespace threadsafe {

template <typename AllocT, typename LockT = std::mutex>
class list_balancer_ext : public list_balancer<AllocT>
{
public:
    list_balancer_ext()
        : list_balancer<AllocT>::list_balancer()
    {  }

    list_balancer_ext(const std::vector<std::pair<AllocT, unsigned>> & _init_list)
        : list_balancer<AllocT>::list_balancer(_init_list)

    {

    }


    void add_unit(AllocT unit_, unsigned pref_)
    {
        std::lock_guard<LockT> lc(_rw_locker);
        this->__add_unit(unit_, pref_);
    }

    const AllocT & balanced_rand()
    {
        std::lock_guard<LockT> lc(_rw_locker);
        return list_balancer<AllocT>::balanced_rand();
    }

    void clear()
    {
        this->__claear();
    }

private:
    LockT _rw_locker;
};

} // namespace ts

} // namespace redis

#endif // LIST_BALANSER_EXT_HPP
