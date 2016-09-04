#ifndef QUERY_HPP
#define QUERY_HPP

#include <string>
#include <vector>
#include <cstring>

#include "types.hpp"
#include "cmd_traits.hpp"
#include "buffers/buff_adapters.hpp"


namespace redis {

// Utils for query builder.
size_t ins_query_prefix(std::string & query_, unsigned pcount_);
void add_query_part(std::string & query_, const char * part_);

std::vector<asio::const_buffer> build_multibuffer(const std::string & query_,
                                                  const DBuffsPosList & _ext_buffs_list);

template <typename CmdType, typename ExtBuffType = redis::buff::common_buffer>
class  query
{
public:

    // Constructor overload for redis commands with 1 arguments.
    template <typename T = CmdType, typename BType = ExtBuffType,
              typename = std::enable_if_t<T::params_count == 1 &&
                                          std::is_same<BType, redis::buff::common_buffer>::value
                                          >
              >
    explicit query(const std::string key_)
        : _query(new std::string())

    {
        size_t name_len = std::strlen(CmdType::name);
        auto max_size = query_pref_max_size*2 + name_len + key_.size() + 4;
        _query->resize(max_size);
        size_t added_size = snprintf(&((*_query)[0]), max_size,
                                     "*2\r\n$%d\r\n%s\r\n$%d\r\n%s\r\n",
                                     static_cast<unsigned>(name_len), CmdType::name,
                                     static_cast<unsigned>(key_.size()), key_.c_str());

        _query->resize(added_size);
        return;
    }

    // Constructor overload for redis commands with 2 arguments.
    template <typename T = CmdType, typename BType = ExtBuffType,
              typename = std::enable_if_t<T::params_count == 2 &&
                                          std::is_same<BType, redis::buff::common_buffer>::value
                                          >
              >
    explicit query(const std::string & key_, const std::string & value_)
        : _query(new std::string())
    {
        size_t name_len = std::strlen(CmdType::name);
        auto max_size = query_pref_max_size*3 + key_.size() + value_.size() + 6;
        _query->resize(max_size);
        size_t added_size = snprintf(&((*_query)[0]), max_size,
                                     "*3\r\n$%d\r\n%s\r\n$%d\r\n%s\r\n$%d\r\n%s\r\n",
                                     static_cast<unsigned>(name_len), CmdType::name,
                                     static_cast<unsigned>(key_.size()), key_.c_str(),
                                     static_cast<unsigned>(value_.size()), value_.c_str()
                );

        _query->resize(added_size);
        return;
    }

    // Constructor overload for queryes without arguments.
    template <typename T = CmdType,
              typename = std::enable_if_t<T::params_count == 0>
              >
    explicit query()
        : _query(new std::string())
    {
        // Build query. See description in overload below.
        _query->resize(query_pref_max_size);
        _build_RESP_next(CmdType::name);
        ins_query_prefix(*_query, _pcount);
    }

    // Overload for queryes with arguments.
    template <typename T = CmdType,
              typename ...Args,
              typename = std::enable_if_t<T::params_count < 0 ||
                                          std::is_same<ExtBuffType, redis::buff::direct_write_buffer>::value>
              >
    explicit query(Args && ...args)
        : _query(new std::string())

    {
        // Count of all parameters for redis query will be known only after _build_RESP_query() finished work.
        // For maximum performance and to avoid memory relocation - reserved additional memory for
        // array lenth in the begining of query string.
        // Maximum text length of redis list size (2^32 - 1) + length of "*\r\n".
        //  unsigned pref_max_size = max_redis_list_size() + 3;
        _query->resize(query_pref_max_size);
        // Debug filling.
        // std::fill(_query.begin(), _query.end(), '0');

        // Recurcive query building.
        _build_RESP_next(CmdType::name, std::forward<Args>(args)...);

        // Add prefix "*%count%\r\n" (see RESP protocol docs).
        size_t trimed = ins_query_prefix(*_query, _pcount);

        // Decreasing external direct buffers offsets.
        for (auto & pos : _ext_buffs_list)
            pos.first -= trimed;

    }

    const std::string & as_string_ref() const
    {
        return *_query;
    }

    const char * qdata() const
    {
        return _query->c_str();
    }

    size_t qsize() const
    {
        return _query->size();
    }


    std::vector<asio::const_buffer> get_multibuffer() const
    {
        return build_multibuffer(*_query, _ext_buffs_list);
    }

protected:
    unsigned _pcount{0};
    std::shared_ptr<std::string> _query;
    DBuffsPosList _ext_buffs_list;

    friend class serial_query_adapter;

    // Recursive query builder.
    template <typename ...Args>
    void _build_RESP_next(const std::string & part_, Args && ...args)
    {
        _build_RESP_next(part_.c_str());
        _build_RESP_next(std::forward<Args>(args)...);
    }

    inline void _build_RESP_next(const std::string & part_)
    {
        _build_RESP_next(part_.c_str());
    }

    template <typename ...Args>
    void _build_RESP_next(const char * part_, Args && ...args)
    {
        _build_RESP_next(part_);
        _build_RESP_next(std::forward<Args>(args)...);
    }

    inline void _build_RESP_next(const char * part_)
    {
        add_query_part(*_query, part_);
        ++_pcount;
    }

    template <typename ...Args>
    void _build_RESP_next(const std::vector<std::string> & part_, Args && ...args)
    {
        _build_RESP_next(part_);
        _build_RESP_next(std::forward<Args>(args)...);
    }

    inline void _build_RESP_next(const std::vector<std::string> & part_)
    {
        for (auto & str : part_)
            _build_RESP_next(str.c_str());
    }

    template <typename ...Args>
    void _build_RESP_next(int part_, Args && ...args)
    {
        _build_RESP_next(part_);
        _build_RESP_next(std::forward<Args>(args)...);
    }

    inline void _build_RESP_next(int part_)
    {
        _build_RESP_next(std::to_string(part_).c_str());
    }

    template <typename T, typename ...Args, typename CT = CmdType, typename BT = ExtBuffType,
              typename = std::enable_if_t<CT::enable_direct_write_buff &&
                                          std::is_same<BT, buff::direct_write_buffer>::value
                                          >
              >
    void _build_RESP_next(const redis::buff::output_adapter<T> & _direct_buff, Args && ...args)
    {
        _build_RESP_next(_direct_buff);
        _build_RESP_next(std::forward<Args>(args)...);
    }

    template <typename T, typename ...Args, typename CT = CmdType, typename BT = ExtBuffType,
              typename = std::enable_if_t<CT::enable_direct_write_buff &&
                                          std::is_same<BT, buff::direct_write_buffer>::value
                                          >
              >
    inline void _build_RESP_next(const redis::buff::output_adapter<T> & direct_buff_)
    {
        // query_pref_max_size + additional "/r/n"
        char pref[query_pref_max_size];

        size_t sz = snprintf(pref, query_pref_max_size + 2, "$%d\r\n\r\n", static_cast<unsigned>(direct_buff_.size()));
        _query->append(pref, sz);

        _ext_buffs_list.push_back(std::pair<size_t, asio::const_buffer>(_query->size() - 2,
                                                                        asio::buffer(direct_buff_.data(),
                                                                                     direct_buff_.size())));
        ++_pcount;
    }
};


class serial_query_adapter
{
    redis_callback _cb;
    unsigned _pcount{0};
    std::shared_ptr<std::string> _query;
    DBuffsPosList _ext_buffs_list;
public:

    serial_query_adapter()
    {

    }

    template <typename qT, typename cbType>
    explicit serial_query_adapter(const qT & qu_, cbType && cb_)
        : _cb(std::forward<cbType>(cb_)),
          _pcount(qu_._pcount),
          _query(qu_._query),
          _ext_buffs_list(qu_._ext_buffs_list)
    {

    }

    template <typename ...Args>
    void callback(Args && ...args) const
    {
        _cb(std::forward<Args>(args)...);
    }

    std::vector<asio::const_buffer> get_multibuffer() const
    {
        return build_multibuffer(*_query, _ext_buffs_list);
    }

    const std::string & as_str_ref() const
    {
        return *_query;
    }

    redis_callback get_callback() const
    {
        return _cb;
    }

};

} // namespace redis

#endif // QUERY_HPP
