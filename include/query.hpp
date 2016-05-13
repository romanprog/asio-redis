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

    using query_cb_type = std::function<void (rds_err, const typename CmdType::return_type &)>;

    // Overload for queryes without arguments.
    template <typename T = CmdType,
              typename = std::enable_if_t<T::no_params &&
                                          !std::is_base_of<cmd::one_line, T>::value>
              >
    explicit query(query_cb_type cb)
        : _cb(cb),
          _query(new std::string())
    {
        // Build query. See description in overload below.
        _query->resize(query_pref_max_size);
        _build_RESP_next(CmdType::name);
        ins_query_prefix(*_query, _pcount);

    }
    // Overload for queryes with arguments.
    template <typename T = CmdType,
              typename ...Args,
              typename = std::enable_if_t<!T::no_params &&
                                          (sizeof ...(Args) > 0)>
              >
    explicit query(query_cb_type cb, Args && ...args)
        : _cb(cb),
          _query(new std::string())

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

        // Add prefix "*count\r\n" (see RESP protocol docs).
        size_t trimed = ins_query_prefix(*_query, _pcount);

        // Decreasing external direct buffers offsets.
        for (auto & pos : _ext_buffs_list)
            pos.first -= trimed;

    }

    // Overload for simple one-line query(like "incr test" or "set test 1") with string as parameter;
    template <typename T = CmdType,
              typename = std::enable_if_t<T::no_params &&
                                          std::is_base_of<cmd::one_line, T>::value>
              >
    explicit query(query_cb_type cb, std::string query_)
        : _cb(cb),
          _query(new std::string(std::move(query_)))
    {
        *_query += "\r\n";
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

    auto get_callback() const
    {
        return _cb;
    }

    template <typename ...Args>
    void callback(Args && ...args)
    {
        _cb(std::forward<Args>(args)...);
    }

    std::vector<asio::const_buffer> get_multibuffer() const
    {
        return build_multibuffer(*_query, _ext_buffs_list);
    }

protected:
    query_cb_type _cb;
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
              typename = std::enable_if_t<CT::enable_direct_send_buff &&
                                          std::is_same<BT, buff::direct_write_buffer>::value
                                          >
              >
    void _build_RESP_next(const redis::buff::output_adapter<T> & _direct_buff, Args && ...args)
    {
        _build_RESP_next(_direct_buff);
        _build_RESP_next(std::forward<Args>(args)...);
    }

    template <typename T, typename ...Args, typename CT = CmdType, typename BT = ExtBuffType,
              typename = std::enable_if_t<CT::enable_direct_send_buff &&
                                          std::is_same<BT, buff::direct_write_buffer>::value
                                          >
              >
    inline void _build_RESP_next(const redis::buff::output_adapter<T> & direct_buff_)
    {
        // query_pref_max_size + additional "/r/n"
        char pref[query_pref_max_size + 2];

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
    std::function<void (rds_err, resp_data &)> _cb;
    unsigned _pcount{0};
    std::shared_ptr<std::string> _query;
    DBuffsPosList _ext_buffs_list;

public:
    template <typename qT>
    explicit serial_query_adapter(const qT & qu_)
        : _cb(qu_._cb),
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

};

} // namespace redis

#endif // QUERY_HPP
