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

void q_fast_build(std::string & query_, const char * const cmd_);
void q_fast_build(std::string & query_, const char * const cmd_, const char * const key_);
void q_fast_build(std::string & query_, const char * const cmd_, const char * const key_, const char * const val_);

std::vector<asio::const_buffer> build_multibuffer(const std::string & query_,
                                                  const qbuff_pos_list & _ext_buffs_list);

template <typename CmdType, typename ExtBuffType = redis::buff::common_buffer>
class  query
{
public:

    // Section of fast constructor overloads.
    // For the most common use.
    template <typename T = CmdType,
              typename = std::enable_if_t<!std::is_same<T, redis::cmd::custom>::value>
              >
    query()
        : _query(new std::string())
    {
        q_fast_build(*_query, T::name);
    }

    template <typename T = CmdType,
              typename = std::enable_if_t<!std::is_same<T, redis::cmd::custom>::value>
              >
    query(const std::string & key_)
        : _query(new std::string())
    {
        q_fast_build(*_query, T::name, key_.c_str());
    }

    template <typename T = CmdType,
              typename = std::enable_if_t<!std::is_same<T, redis::cmd::custom>::value>
              >
    query(const char * const key_)
        : _query(new std::string())
    {
        q_fast_build(*_query, T::name, key_);
    }

    template <typename T = CmdType,
              typename = std::enable_if_t<!std::is_same<T, redis::cmd::custom>::value>
              >
    query(const std::string & key_, const std::string & val_)
        : _query(new std::string())
    {
        q_fast_build(*_query, T::name, key_.c_str(), val_.c_str());
    }

    template <typename T = CmdType,
              typename = std::enable_if_t<!std::is_same<T, redis::cmd::custom>::value>
              >
    query(const char * const key_, const std::string & val_)
        : _query(new std::string())
    {
        q_fast_build(*_query, T::name, key_, val_.c_str());
    }

    template <typename T = CmdType,
              typename = std::enable_if_t<!std::is_same<T, redis::cmd::custom>::value>
              >
    query(const std::string & key_, const char * const val_)
        : _query(new std::string())
    {
        q_fast_build(*_query, T::name, key_.c_str(), val_);
    }

    template <typename T = CmdType,
              typename = std::enable_if_t<!std::is_same<T, redis::cmd::custom>::value>
              >
    query(const char * const key_, const char * const val_)
        : _query(new std::string())
    {
        q_fast_build(*_query, T::name, key_, val_);
    }

    // Default constructor overload.
    template <typename ...Args>
    query(Args && ...args)
        : _query(new std::string())

    {
        if (!std::is_same<CmdType, redis::cmd::custom>::value) {
            std::string tmp(CmdType::name);
            _build_RESP(tmp, std::forward<Args>(args)...);
        } else {
            _build_RESP(std::forward<Args>(args)...);
        }

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
    qbuff_pos_list _ext_buffs_list;

    friend class serial_query_adapter;

    template <typename ...Args,
              typename T = CmdType>
    inline void _build_RESP(Args && ...args)
    {
        // Count of all parameters for redis query will be known only after _build_RESP_query() finished work.
        // For maximum performance and to avoid memory relocation - reserved additional memory for
        // array lenth in the begining of query string.
        // Maximum text length of redis list size (2^32 - 1) + length of "*\r\n".
        _query->resize(query_pref_max_size);

        // Recurcive query building.
        _build_RESP_next(std::forward<Args>(args)...);

        // Add prefix "*%count%\r\n" (see RESP protocol docs).
        size_t trimed = ins_query_prefix(*_query, _pcount);

        // Decreasing external direct buffers offsets.
        for (auto & pos : _ext_buffs_list)
            pos.first -= trimed;
    }

    // Recursive query builder.
    template <typename ...Args>
    void _build_RESP_next(const std::string & part_, Args && ...args)
    {
        _build_RESP_next(part_.c_str());
        _build_RESP_next(std::forward<Args>(args)...);
    }

    void _build_RESP_next(const std::string & part_)
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

    void _build_RESP_next(const std::vector<std::string> & part_)
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

    void _build_RESP_next(int part_)
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

    template <typename T, typename CT = CmdType, typename BT = ExtBuffType,
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
    std::shared_ptr<std::string> _query;
    qbuff_pos_list _ext_buffs_list;

public:

    serial_query_adapter()
    {

    }

    template <typename qT, typename cbType>
    explicit serial_query_adapter(const qT & qu_, cbType && cb_)
        : _cb(std::forward<cbType>(cb_)),
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
