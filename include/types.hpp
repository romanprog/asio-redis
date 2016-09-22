#ifndef TYPES_HPP
#define TYPES_HPP

#include <string>
#include <vector>
#include <memory>
#include <queue>
#include <functional>
#include <mutex>
#include <asio.hpp>

#include "utils/h_net.hpp"

namespace redis {

using rds_err = int;

// Calculate text length of unsigned value at compile time.
template<unsigned long n>
struct TLenCounter {
    enum { value = 1 + TLenCounter<n/10>::value };
};

template <>
struct TLenCounter<0> {
    enum {value = 0};
};

static constexpr unsigned max_redis_list_size()
{
    return TLenCounter<std::numeric_limits<unsigned>::max()>::value;
}
static constexpr size_t query_pref_max_size {max_redis_list_size() + 3};

enum class respond_type
{
    simple_str,
    error_str,
    integer,
    bulk_str,
    array,
    empty
};

struct cl_options
{
    cl_options()
    {

    }

    cl_options(unsigned c_tout_, unsigned r_tout_)
        : conn_timeout(c_tout_),
          resp_timeout(r_tout_)
    {

    }

    // Client connection timeout in milliseconds.
    unsigned conn_timeout {3000};
    // Server respond timeout in milliseconds.
    unsigned resp_timeout {3000};
};


class input_buff;

struct resp_data
{
    respond_type type {respond_type::empty};
    std::string sres;
    int ires {0};
    std::vector<resp_data> ares;
    bool isnull {false};
    void reset()
    {
        ares.clear();
        ires = 0;
        sres.clear();
        isnull = false;
        type = respond_type::empty;
    }
};

struct resp_string
{

};


struct srv_endpoint
{
    srv_endpoint(const std::string & ip_, unsigned port_, unsigned pref_ = 10)
        : ip(ip_),
          port(port_),
          pref(pref_)
    {  }

    std::string ip;
    unsigned port;
    unsigned pref;
    inline bool no_error() const
    {
        return (hnet::is_ip_v4(ip) && pref > 0 && pref <= 10);
    }
};

using resp_data_ptr = std::unique_ptr<resp_data>;
using redis_callback = std::function<void (int, const resp_data &)>;
using qbuff_pos_list = std::vector<std::pair<size_t, asio::const_buffer>>;

using soc_ptr = std::shared_ptr<asio::ip::tcp::socket>;
using strand_ptr = std::shared_ptr<asio::strand>;
using conn_callback = std::function<void (asio::error_code&)>;
using get_soc_callback = std::function<void (asio::error_code&, soc_ptr && result)>;
using get_socs_list_callback = std::function<void (asio::error_code&, std::vector<soc_ptr> && result)>;
using disconection_cb = std::function<void(asio::error_code)>;

} // namespace redis


#endif // TYPES_HPP
