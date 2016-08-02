#include "../include/proto.hpp"
#include "../include/buffers/io_buffers.hpp"

#include "../utils/h_strings.hpp"

#include <cstring>
#include <iostream>
#include <algorithm>

namespace redis {


resp_proto::resp_proto()
    :__buffer(std::make_shared<input_buff>())
{

}

resp_proto::resp_proto(input_buff_ptr buff_)
    :__buffer(buff_)
{ }

std::string resp_proto::error_msg() const
{
    return _err_message;
}

bool resp_proto::have_error()
{
    return _error_status;
}

bool resp_proto::parse_one(redis::resp_data &respond)
{
   __buffer->_comlated = false;

    if (__buffer->top_offset() <= __buffer->_unparsed_offset)
        return false;

    if (!__buffer->_incompl_arr) {

        respond.reset();
        if (_read_data(respond, __buffer->unparsed_data()))
            __buffer->_comlated = true;
    }
    else {
        if (_fill_array(respond)) {
            __buffer->_comlated = true;
            __buffer->_incompl_arr = false;
        }
    }
    if (__buffer->_comlated)
        __buffer->manage_mem();

    return __buffer->_comlated;
}

int resp_proto::parse_string(std::string &data, size_t &ret_bulk_sz)
{

    if (data.empty())
       return false;

    // Can parse only bulk strind data.
    if (data[0] != '$')
        throw std::logic_error("RESP data error. Wrong data type.");

    // $-1\r\n. Null bulk string. (Unexisting key).
    if (data[1] == '-') {
        data.clear();
        return true;
    }
    // $4\r\nABCD\r\n
    // Read string size.
    size_t bulk_size {0};
    size_t i {1};

    ret_bulk_sz = 0;

    while (data[i] != '\r')
    {
        if (i == data.size() - 1)
            return false;

        bulk_size = (bulk_size * 10) + (data[i] - '0');
        ++i;
    }


    ret_bulk_sz = bulk_size;

    if (data.size() < bulk_size + i + 4)
        return false;

    data.erase(data.begin(), data.begin() + i + 2);
    data.resize(bulk_size);
    return true;

}

bool resp_proto::try_bulk_parse(size_t sz, size_t &data_begin, size_t &ret_bulk_sz)
{
    const char * cursor = __buffer->data();
    data_begin = ret_bulk_sz = 0;

    // It's not bulk string.
    if (*cursor != '$')
        return false;

    ++cursor;
    --sz;

    // Check minimal bulk str size.
    if (sz < 4)
        return true;

    // Empty bulk string. Data begin != 0, bulk_size == 0, return true.
    if (*cursor == '-') {
        data_begin = 3;
        return true;
    }

    // Read string size.
    size_t bulk_size {0};
    size_t i {0};
    // $3\r\nABC\r\n sz = 8
    while (cursor[i] != '\r')
    {
        if (!(sz - i))
            return true;

        bulk_size = (bulk_size * 10) + (cursor[i] - '0');
        ++i;
    }

    if (sz - i == bulk_size + 4) {
        data_begin = sz - bulk_size - 1;
        ret_bulk_sz = bulk_size;
    }

    return true;
}

input_buff &resp_proto::buff()
{
    return *__buffer;
}


bool resp_proto::_read_simple_string(resp_data &target, const char * cursor, size_t sz)
{
    size_t i {0};

    while (cursor[i] != '\r')
    {
        if (!(sz - i - 2))
            return false;
        ++i;
    }

#ifdef ADDITIONAL_ERROR_CHECK
    if (cursor[i+1] != '\n') {
        parsing_error_hendler();
        return false;
    }
#endif

    target.sres.assign(cursor, i);
    __buffer->_unparsed_offset = (cursor - __buffer->data()) + i + 2; // 2 = "\r\n" length

    return true;
}

bool resp_proto::_read_integer(resp_data &target, const char *cursor, size_t sz)
{
    target.ires = 0;
    size_t i {0};

    while (cursor[i] != '\r')
    {
        if (!(sz - i - 2))
            return false;
        target.ires = (target.ires * 10) + (cursor[i] - '0');
        ++i;
    }

#ifdef ADDITIONAL_ERROR_CHECK
    if (cursor[i+1] != '\n') {
        parsing_error_hendler();
        return false;
    }
#endif

    target.type = respond_type::integer;
    __buffer->_unparsed_offset = (cursor - __buffer->data()) + i + 2; // 2 = "\r\n" length
    return true;
}

bool resp_proto::_read_bulk_string(resp_data &target, const char *cursor, size_t sz)
{
    // $-1\r\n. Null bulk string. (Unexisting key).
    if (*cursor == '-') {
#ifdef ADDITIONAL_ERROR_CHECK
        if (cursor[1] != '1' || cursor[2] != '\r' || cursor[3] != '\n') {
            parsing_error_hendler();
            return false;
        }
#endif
        target.isnull = true;
        __buffer->_unparsed_offset = (cursor - __buffer->data()) + 4;
        return true;
    }
    // Read string size.
    size_t bulk_size {0};
    size_t i {0};

    while (cursor[i] != '\r')
    {
        if (!(sz - i))
            return false;
        bulk_size = (bulk_size * 10) + (cursor[i] - '0');
        ++i;
    }

    if (sz - i < bulk_size + 4)
        return false;

#ifdef ADDITIONAL_ERROR_CHECK
    if (cursor[bulk_size + i + 3] != '\n') {
        parsing_error_hendler();
        return false;
    }
#endif
    // Set cursor to begining of next part.
    target.sres.assign(cursor + i + 2, bulk_size);
    target.type = respond_type::bulk_str;
    __buffer->_unparsed_offset = (cursor - __buffer->data()) + bulk_size + i + 4;
    return true;

}

bool resp_proto::_init_array(resp_data &target, const char *cursor, size_t sz)
{
    // *0\r\n. Empty array.
    if (*cursor == '0') {
#ifdef ADDITIONAL_ERROR_CHECK
        if (cursor[1] != '\r' || cursor[2] != '\n') {
            parsing_error_hendler();
            return false;
        }
#endif
        target.isnull = true;
        __buffer->_unparsed_offset = (cursor - __buffer->data()) + 3;
        return true;
    }
    // Read array size.
    size_t array_size {0};
    size_t i {0};

    while (cursor[i] != '\r')
    {
        if (!(sz - i))
            return false;
        array_size = (array_size * 10) + (cursor[i] - '0');
        ++i;
    }

#ifdef ADDITIONAL_ERROR_CHECK
    if (cursor[i + 1] != '\n') {
        parsing_error_hendler();
        return false;
    }
#endif
    // Resize @target array and call _fill_array().
    target.ares.resize(array_size);
    __buffer->_unparsed_offset = (cursor - __buffer->data()) + i + 2;

    if (!_fill_array(target)) {
        __buffer->_incompl_arr = true;
        return false;
    }
    return true;
}

bool resp_proto::_fill_array(resp_data &target)
{
    // Recursive check of each @target record. Searching incomplated array.

    // Array empty - return.
    if (target.ares.empty())
        return true;

    // Have no data for parsing.
    if (!__buffer->unparsed_size())
        return false;

    // Check array.
    for (int i = 0; i < target.ares.size(); ++i)
    {
        // If found "array in array" - run recursive check.
        if (target.ares[i].type == respond_type::array)
        {
            if (!_fill_array(target.ares[i]))
                return false;

            continue;
        }
        // Target have value - check next
        else if (target.ares[i].type != respond_type::empty)
            continue;
        // Target empty - run base data reading method (_read_data()) for nested data.
        if (!_read_data(target.ares[i], __buffer->unparsed_data()))
            return false;
    }
    return true;
}

bool resp_proto::_read_data(resp_data &target, const char *cursor)
{

    switch (*cursor) {
    case '+':
    {
        // Simple string. Simple parsing.

        if (__buffer->unparsed_size() < 4)
            return false; // Incomplate reply.

        if (_read_simple_string(target, ++cursor, __buffer->unparsed_size() - 1)) {
            target.type = respond_type::simple_str;
            return true;
        }

        break;
    }
    case '-':
    {
        if (__buffer->unparsed_size() < 4)
            return false; // Incomplate reply.

        // Simple string with error message.
        if (_read_simple_string(target, ++cursor, __buffer->unparsed_size() - 1)) {
            target.type = respond_type::error_str;
            return true;
        }

        break;
    }
    case ':':
    {
        if (__buffer->unparsed_size() < 4)
            return false; // Incomplate reply.

        if (_read_integer(target, ++cursor, __buffer->unparsed_size() - 1))
            return true;

        break;
    }
    case '$':
    {
        // Bulk string.
        if (__buffer->unparsed_size() < 5)
            return false; // Incomplate reply.

        if (_read_bulk_string(target, ++cursor, __buffer->unparsed_size() - 1)) {
            target.type = respond_type::bulk_str;
            return true;
        }
        break;
    }
    case '*':
        // Array.
        if (__buffer->unparsed_size() < 4)
            return false; // Incomplate reply.

        if (_init_array(target, ++cursor, __buffer->unparsed_size() - 1))
            return true;

        break;
    default:
        parsing_error_hendler();
    }

    return false;
}

void resp_proto::parsing_error_hendler()
{
    _error_status = true;
    _err_message = "Reply data parsing error.";

    throw std::logic_error(_err_message);
}

} //namespace redis
