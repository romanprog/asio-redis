#ifndef PROTO_HPP
#define PROTO_HPP

// Additional error check. (The correctness of the protocol.)
// #define ADDITIONAL_ERROR_CHECK

#include "buffers/io_buffers.hpp"
#include "types.hpp"

#include <string>
#include <vector>
#include <mutex>

/// ///////////////////////////////////////////////////////////////////////////
/// \brief The RESProto class is a RESP (REdis Serialization Protocol) parser.
/// Have simple interface.
/// Use parse_one() in cycle whith RespData reference as param.
/// Important:
/// If there is a probability of receiving incomplete data (parted) -
/// RespData variable should not be reseted after the call parse_one().
/// Next call parse_one() will update it with new data
/// ////////////////////////////////////////////////////////////////////////////

namespace redis {

using input_buff_ptr = std::shared_ptr<input_buff>;

class resp_proto
{
public:

    resp_proto();
    resp_proto(input_buff_ptr buff_);

    // Return error message.
    std::string error_msg() const;
    // Error status.
    bool have_error();
    // Parse data. When first complated  RESP protocol part parsed - return
    // true and write result to @respond. When part incomplate - parse all that
    // posible, write results to @respond and return false.
    bool parse_one(redis::resp_data & respond);

    static int parse_string(std::string & data);

    input_buff & buff();

private:

    // RESP types parsers:
    // Simple string.
    inline bool _read_simple_string(redis::resp_data &target, const char *cursor, size_t sz);
    // Int value.
    inline bool _read_integer(redis::resp_data &target, const char *cursor, size_t sz);
    // Bulk string.
    bool _read_bulk_string(redis::resp_data &target, const char *cursor, size_t sz);
    // Array.
    bool _init_array(redis::resp_data &target, const char *cursor, size_t sz);
    // Complate parsing of parted array (recursive).
    bool _fill_array(redis::resp_data &target);

    // Base parser function. Detect type of RESP data. Start parser.
    bool _read_data(redis::resp_data &target, const char *cursor);

    // Error handling. Throw runing!
    inline void parsing_error_hendler();
    // Memory management: cleaning, fast reset, data transfer on free sites
    // to avoid the appearance of a new memory.


    std::string _err_message;
    bool _error_status {false};

    input_buff_ptr __buffer;

};

} // namespace redis

#endif // PROTO_HPP
