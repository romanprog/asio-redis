#ifndef LIST_BALANSER_HPP
#define LIST_BALANSER_HPP

#include <memory>
#include <functional>
#include <mutex>
#include <queue>
#include <atomic>

namespace redis {

namespace threadsafe {

template <typename AllocT>
class list_balancer
{
public:

    list_balancer(const std::vector<std::pair<AllocT, unsigned>> & _init_list)
    {
        // Allocate memory for map (100 units max).
        _map = static_cast<unsigned char *>(malloc(1000));
        // Randomize.
        srand(time(0));
        // Add units.
        for (auto & unit : _init_list)
            __add_unit(unit.first, unit.second);

    }

    ~list_balancer()
    {
        free(_map);
    }

    const AllocT & balanced_rand() const
    {
        return _list[_map[rand() % (_map_size)]].first;
    }

    const bool empty() const
    {
        return _list.empty();
    }

    const size_t size() const
    {
        return _list.size();
    }

    const std::vector<std::pair<AllocT, unsigned>> & get_list() const
    {
        return _list;
    }

protected:

    list_balancer()
    {
        _map = static_cast<unsigned char *>(malloc(1000));

        srand(time(0));

    }

    void __add_unit(AllocT unit_, unsigned pref_)
    {
        if (pref_ < 1 || pref_ > 10 )
            throw std::logic_error("Preference value out of range (1-10).");

        if (_list.size() >= 100)
            throw std::logic_error("Pool units count out of range (max - 100).");

        _list.push_back(std::pair<AllocT, unsigned>(unit_, pref_));
        unsigned last_index = _list.size() - 1;
        for (int i = _map_size; i < _map_size + pref_; ++i)
            _map[i] = last_index;

         _map_size += pref_;
    }

    void __claear()
    {
        _list.clear();
    }

private:
    std::vector<std::pair<AllocT, unsigned>> _list;
    unsigned _map_size {0};
    unsigned char * _map {nullptr};
};

} // namespace ts

} // namespace redis
#endif // LIST_BALANSER_HPP
