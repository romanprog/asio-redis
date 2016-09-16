#ifndef COMMANDSTRAITS_HPP
#define COMMANDSTRAITS_HPP

#include "types.hpp"

namespace redis {
namespace cmd {

// Base set of command trains and properties.
struct default_traits
{
    static constexpr bool is_blocking {false};
    static constexpr bool enable_direct_write_buff {false};
    using return_type = resp_data;
    using only_master_t = std::true_type;
    static constexpr int params_count = -1;};
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
    static constexpr int params_count = 0;
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
    static constexpr int params_count = 0;
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
    static constexpr int params_count = 0;
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
    static constexpr int params_count = 0;
    static constexpr char const * name {"READONLY"};
};

// READWRITE
struct readwrite : public default_traits
{
    static constexpr int params_count = 0;
    static constexpr char const * name {"READWRITE"};
};

} // namespace claster
////////////////////////////////////////////////////////////////////////


/// //////////////////  CONNECTION COMMANDS ////////////////////////////
namespace conn {

// AUTH password
struct auth : public default_traits
{
    static constexpr char const * name {"AUTH"};
};

} // namespace conn
////////////////////////////////////////////////////////////////////////

/// //////////////////////  GEO COMMANDS ///////////////////////////////

namespace geo {

// GEOADD key longitude latitude member [longitude latitude member ...]
struct geoadd : public default_traits
{
    static constexpr char const * name {"GEOADD"};
};

// GEOHASH key member [member ...]
struct geohash : public default_traits
{
    static constexpr char const * name {"GEOHASH"};
};

// GEOPOS key member [member ...]
struct geopos : public default_traits
{
    static constexpr char const * name {"GEOPOS"};
};

// GEORADIUS key longitude latitude radius m|km|ft|mi [WITHCOORD] [WITHDIST] [WITHHASH] [COUNT count] [ASC|DESC] [STORE key] [STOREDIST key]
struct georadius : public default_traits
{
    static constexpr char const * name {"GEORADIUS"};
};

// GEORADIUSBYMEMBER key member radius m|km|ft|mi [WITHCOORD] [WITHDIST] [WITHHASH] [COUNT count] [ASC|DESC] [STORE key] [STOREDIST key]
struct georadiusbymember : public default_traits
{
    static constexpr char const * name {"GEORADIUSBYMEMBER"};
};

} // namsepace geo
/// ////////////////////////////////////////////////////////////////////

/// ////////////////////  HASH COMMANDS ////////////////////////////////

namespace hash {

// HDEL key field [field ...]
struct hdel : public default_traits
{
    static constexpr char const * name {"HDEL"};
};

// HEXISTS key field
struct hexists : public default_traits
{
    using only_master_t = std::false_type;
    static constexpr int params_count = 2;
    static constexpr char const * name {"HEXISTS"};
};

// HGET key field
struct hget : public default_traits
{
    using only_master_t = std::false_type;
    static constexpr int params_count = 2;
    static constexpr char const * name {"HGET"};
};

// HGETALL key
struct hgetall : public default_traits
{
    using only_master_t = std::false_type;
    static constexpr int params_count = 1;
    static constexpr char const * name {"HGETALL"};
};

// HINCRBY key field increment
struct hincrby: public default_traits
{
    static constexpr char const * name {"HINCRBY"};
};

// HINCRBYFLOAT key field increment
struct hincrbyfloat: public default_traits
{
    static constexpr char const * name {"HINCRBYFLOAT"};
};

// HKEYS key
struct hkeys: public default_traits
{
    using only_master_t = std::false_type;
    static constexpr int params_count = 1;
    static constexpr char const * name {"HKEYS"};
};

// HLEN key
struct hlen: public default_traits
{
    using only_master_t = std::false_type;
    static constexpr int params_count = 1;
    static constexpr char const * name {"HLEN"};
};

//HMGET key field [field ...]
struct hmget: public default_traits
{
    using only_master_t = std::false_type;
    static constexpr char const * name {"HMGET"};
};

// HMSET key field value [field value ...]
struct hmset: public default_traits
{
    static constexpr bool enable_direct_write_buff {true};
    static constexpr char const * name {"HMSET"};
};

// HSCAN key cursor [MATCH pattern] [COUNT count]
struct hscan: public default_traits
{
    using only_master_t = std::false_type;
    static constexpr char const * name {"HSCAN"};
};

// HSET key field value
struct hset: public default_traits
{
    static constexpr bool enable_direct_write_buff {true};
    static constexpr char const * name {"HSET"};
};

// HSETNX key field value
struct hsetnx: public default_traits
{
    static constexpr bool enable_direct_write_buff {true};
    static constexpr char const * name {"HSETNX"};
};

// HSTRLEN key field
struct hstrlen: public default_traits
{
    using only_master_t = std::false_type;
    static constexpr int params_count = 2;
    static constexpr char const * name {"HSTRLEN"};
};

// HVALS key
struct hvals: public default_traits
{
    using only_master_t = std::false_type;
    static constexpr int params_count = 1;
    static constexpr char const * name {"HVALS"};
};

} // namespace hash
/// ////////////////////////////////////////////////////////////////////

/// ///////////////// Hyper Long Long //////////////////////////////////
namespace hll {

// PFADD key element [element ...]
struct pfadd: public default_traits
{
    static constexpr bool enable_direct_write_buff {true};
    static constexpr char const * name {"PFADD"};
};

// PFCOUNT key [key ...]
struct pfcount: public default_traits
{
    using only_master_t = std::false_type;
    static constexpr char const * name {"PFCOUNT"};
};

// PFMERGE destkey sourcekey [sourcekey ...]
struct pfmerge: public default_traits
{
    static constexpr char const * name {"PFMERGE"};
};

} // namespace hll
/// ////////////////////////////////////////////////////////////////////

/// //////////////////// Keys //////////////////////////////////////////


namespace key
{
// DEL key [key ...]
struct del: public default_traits
{
    static constexpr char const * name {"DEL"};
};

// DUMP key
struct dump: public default_traits
{
    using only_master_t = std::false_type;
    static constexpr char const * name {"DUMP"};
};

// EXISTS key [key ...]
struct exists: public default_traits
{
    using only_master_t = std::false_type;
    static constexpr char const * name {"EXISTS"};
};

// EXPIRE key seconds
struct expire: public default_traits
{
    static constexpr int params_count = 2;
    static constexpr char const * name {"EXPIRE"};
};

// EXPIREAT key timestamp
struct expireat: public default_traits
{
    static constexpr int params_count = 2;
    static constexpr char const * name {"EXPIREAT"};
};

// KEYS pattern
struct keys: public default_traits
{
    static constexpr int params_count = 1;
    using only_master_t = std::false_type;
    static constexpr char const * name {"KEYS"};
};

// MIGRATE host port key|"" destination-db timeout [COPY] [REPLACE] [KEYS key [key ...]]
struct migrate: public default_traits
{
    static constexpr char const * name {"MIGRATE"};
};

// MOVE key db
struct move: public default_traits
{
    static constexpr int params_count = 2;
    static constexpr char const * name {"MOVE"};
};

// OBJECT subcommand [arguments [arguments ...]]
struct object: public default_traits
{
    using only_master_t = std::false_type;
    static constexpr char const * name {"OBJECT"};
};

// PERSIST key
struct persist: public default_traits
{
    static constexpr int params_count = 1;
    static constexpr char const * name {"PERSIST"};
};

// PEXPIRE key milliseconds
struct pexpire: public default_traits
{
    static constexpr int params_count = 2;
    static constexpr char const * name {"PEXPIRE"};
};

// PEXPIREAT key milliseconds-timestamp
struct pexpireat: public default_traits
{
    static constexpr int params_count = 2;
    static constexpr char const * name {"PEXPIREAT"};
};

// PTTL key
struct pttl: public default_traits
{
    static constexpr int params_count = 1;
    using only_master_t = std::false_type;
    static constexpr char const * name {"PTTL"};
};

// RANDOMKEY
struct randomkey: public default_traits
{
    static constexpr int params_count = 0;
    using only_master_t = std::false_type;
    static constexpr char const * name {"RANDOMKEY"};
};

// RENAME key newkey
struct rename: public default_traits
{
    static constexpr int params_count = 2;
    static constexpr char const * name {"RENAME"};
};

// RENAMENX key newkey
struct renamenx: public default_traits
{
    static constexpr int params_count = 2;
    static constexpr char const * name {"RENAMENX"};
};

// RESTORE key ttl serialized-value [REPLACE]
struct restore: public default_traits
{
    static constexpr char const * name {"RESTORE"};
};

// SCAN cursor [MATCH pattern] [COUNT count]
struct scan: public default_traits
{
    static constexpr char const * name {"SCAN"};
};

// SORT key [BY pattern] [LIMIT offset count] [GET pattern [GET pattern ...]] [ASC|DESC] [ALPHA] [STORE destination]
struct sort: public default_traits
{
    static constexpr char const * name {"SORT"};
};

// TTL key
struct ttl: public default_traits
{
    using only_master_t = std::false_type;
    static constexpr char const * name {"TTL"};
};

// TYPE key
struct type: public default_traits
{
    using only_master_t = std::false_type;
    static constexpr char const * name {"TYPE"};
};

// WAIT numslaves timeout
struct wait: public default_traits
{
    static constexpr bool is_blocking {true};
    static constexpr char const * name {"TYPE"};
};

}

/// ////////////////////////////////////////////////////////////////////

// APPEND key value
struct append : public default_traits
{
    static constexpr bool enable_direct_write_buff {true};
    static constexpr char const * name {"APPEND"};
};

// BGREWRITEAOF
struct bgrewriteaof : public default_traits
{
    static constexpr int params_count = 0;
    static constexpr char const * name {"BGREWRITEAOF"};
};
// Special query type. Use command name as first parameter, wheh
// create redis::query (redis::query<redis::cmd::custom>("set", "key_name", "value", ...)).
// Slave by default in "client::send" function. Use "client::send_master" if needed.
struct custom : public default_traits
{
    static constexpr char const * name {""};
    static constexpr int params_count = 0;
};


struct set : public default_traits
{
    static constexpr char const * name {"set"};
    static constexpr bool enable_direct_write_buff {true};
    using only_master_t = std::true_type;
    static constexpr int params_count = 2;
};

struct incr : public default_traits
{
    static constexpr char const * name {"incr"};
    static constexpr int params_count = 1;
};

struct get : public default_traits
{
    static constexpr char const * name {"get"};
    using only_master_t = std::false_type;
    static constexpr int params_count = 1;
};

struct blpop : public default_traits
{
    static constexpr bool is_blocking {true};
    static constexpr char const * name {"blpop"};
};


} // namespace cmd
// tmp
namespace buff {

struct common_buffer
{
};

struct direct_write_buffer
{
};

} // namespace buuf
} // namespace redis


#endif // COMMANDSTRAITS_HPP
