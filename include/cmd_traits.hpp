#ifndef COMMANDSTRAITS_HPP
#define COMMANDSTRAITS_HPP

#include "types.hpp"

namespace redis {
namespace cmd {

// Base set of command trains and properties.
struct default_traits
{
    static constexpr bool is_blocking {false};
    static constexpr bool enable_direct_send_buff {false};
    static constexpr bool enable_direct_recive_buff {false};
    static constexpr bool no_params {false};
    static constexpr bool only_master{true};
    using return_type = resp_data;
    using only_master_t = std::true_type;
};
/// //////////////////  CLUSTER COMMANDS //////////////////////////////
namespace cluster {

// CLUSTER ADDSLOTS slot [slot ...]
struct cl_addslot : public default_traits
{
    static const std::vector<std::string> name;
};

// CLUSTER COUNT-FAILURE-REPORTS node-id
struct cl_cf_reports : public default_traits
{
    static const std::vector<std::string> name;
};

// CLUSTER COUNTKEYSINSLOT slot
struct cl_countkeysinslot : public default_traits
{
    static const std::vector<std::string> name;
};

// CLUSTER DELSLOTS slot [slot ...]
struct cl_delslots : public default_traits
{
    static const std::vector<std::string> name;
};

// CLUSTER FAILOVER [FORCE|TAKEOVER]
struct cl_failover : public default_traits
{
    static const std::vector<std::string> name;
};

// CLUSTER FORGET node-id
struct cl_forget : public default_traits
{
    static const std::vector<std::string> name;
};

// CLUSTER GETKEYSINSLOT slot count
struct cl_getkeysinslot : public default_traits
{
    static const std::vector<std::string> name;
};

// CLUSTER INFO
struct cl_info : public default_traits
{
    static const std::vector<std::string> name;
    static constexpr bool no_params {true};
};

// CLUSTER KEYSLOT key
struct cl_keyslot : public default_traits
{
    static const std::vector<std::string> name;
};

// CLUSTER MEET ip port
struct cl_meet : public default_traits
{
    static const std::vector<std::string> name;
};

// CLUSTER NODES
struct cl_nodes : public default_traits
{
    static const std::vector<std::string> name;
    static constexpr bool no_params {true};
};

// CLUSTER REPLICATE node-id
struct cl_replicate : public default_traits
{
    static const std::vector<std::string> name;
};

// CLUSTER RESET [HARD|SOFT]
struct cl_reset : public default_traits
{
    static const std::vector<std::string> name;
};

// CLUSTER SAVECONFIG
struct cl_saveconfig : public default_traits
{
    static const std::vector<std::string> name;
    static constexpr bool no_params {true};
};

// CLUSTER SET-CONFIG-EPOCH config-epoch
struct cl_set_config_epoch : public default_traits
{
    static const std::vector<std::string> name;
};

// CLUSTER SETSLOT slot IMPORTING|MIGRATING|STABLE|NODE [node-id]
struct cl_set_slot : public default_traits
{
    static const std::vector<std::string> name;
};

// CLUSTER SLAVES node-id
struct cl_slaves : public default_traits
{
    static const std::vector<std::string> name;
};

// READONLY
struct readonly : public default_traits
{
    static constexpr bool no_params {true};
    static constexpr auto name {"READONLY"};
};

// READWRITE
struct readwrite : public default_traits
{
    static constexpr bool no_params {true};
    static constexpr auto name {"READWRITE"};
};

} // namespace claster
////////////////////////////////////////////////////////////////////////


/// //////////////////  CONNECTION COMMANDS ////////////////////////////
namespace conn {

// AUTH password
struct auth : public default_traits
{
    static constexpr auto name {"AUTH"};
};

} // namespace conn
////////////////////////////////////////////////////////////////////////

// APPEND key value
struct append : public default_traits
{
    static constexpr bool enable_direct_send_buff {true};
    static constexpr auto name {"APPEND"};
};

// BGREWRITEAOF
struct bgrewriteaof : public default_traits
{
    static constexpr auto name {"BGREWRITEAOF"};
    static constexpr bool no_params {true};
};

struct one_line : public default_traits
{
    static constexpr bool is_blocking {false};
    static constexpr bool enable_direct_recive_buff {true};
    static constexpr bool no_params {true};
};


struct set : public default_traits
{
    static constexpr auto name {"set"};
    static constexpr bool enable_direct_send_buff {true};
    using only_master_t = std::true_type;
};

struct incr : public default_traits
{
    static constexpr auto name {"incr"};
};

struct get : public default_traits
{
    static constexpr auto name {"get"};
};

struct blpop : public default_traits
{
    static constexpr bool is_blocking {true};
    static constexpr auto name {"blpop"};
};


} // namespace cmd
// tmp
namespace buff {

struct common_buffer
{
};

struct direct_read_buffer
{
};

struct direct_write_buffer
{
};

} // namespace buuf
} // namespace redis


#endif // COMMANDSTRAITS_HPP
