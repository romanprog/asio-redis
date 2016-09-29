#include "../include/buffers/io_buffers.hpp"

namespace redis {


input_buff::input_buff()
{

}

void input_buff::when_reseted()
{
    _unparsed_offset = 0;
}

size_t input_buff::unparsed_size()
{
    int res = top_offset() - _unparsed_offset;
    if (res < 0)
        return 0;
    return res;
}

const char *input_buff::unparsed_data()
{
    return data() + _unparsed_offset;
}

void input_buff::manage_mem()
{
    if (_unparsed_offset == top_offset()) {
        reset(true);
        return;
    }

    if (size_reserved() > size_filled() * 2)
        return;

    if ((top_offset() - _unparsed_offset) > _unparsed_offset)
        return;

    memcpy(vdata(), data() + _unparsed_offset, top_offset() - _unparsed_offset);
    change_data_top(top_offset() - _unparsed_offset);
    _unparsed_offset = 0;
}

/// ////////////////////////  Output buffer /////////////////////////////////////
output_buff::output_buff()
{

}

output_buff::~output_buff()
{
    if (_have_frozen_buffer)
        free(_frozen_data);
}

bool output_buff::nothing_to_send()
{
    // std::lock_guard<std::mutex> _mem_lock(_memlock_mux);
    return (!top_offset()) && !_have_frozen_buffer;
}

void output_buff::sending_report(size_t bytes_sended)
{
    _frozen_sended_offset += bytes_sended;
    if (_frozen_sended_offset >= _frozen_top_offset) {

        _have_frozen_buffer = false;
        _frozen_sended_offset = 0;
        _frozen_top_offset = 0;
        free(_frozen_data);
    }
}


void output_buff::add_query(const std::string &query, bool plus_rn)
{
    std::lock_guard<std::mutex> _mem_lock(_memlock_mux);
    if (!plus_rn) {
        *this << query;
    } else
    {
        release(query.size() + 2);
        memcpy(data_top(), query.data(), query.size());
        memcpy(static_cast<char*>(data_top()) + query.size(), "\r\n", 2);
        accept(query.size() + 2);
    }

}

bool output_buff::check_overflow(size_t need_write)
{
    std::lock_guard<std::mutex> _mem_lock(_memlock_mux);
    return ((top_offset() + need_write) >= max_buff_size);
}

void output_buff::reset()
{
    // std::lock_guard<std::mutex> _mem_lock(_memlock_mux);
    buff_abstract::reset();
    _frozen_sended_offset = 0;
    _frozen_top_offset = 0;

    if (_have_frozen_buffer)
        free(_frozen_data);

    _have_frozen_buffer = false;
}

asio::const_buffers_1 output_buff::get_buffer()
{

    if (!_have_frozen_buffer) {
        std::lock_guard<std::mutex> _mem_lock(_memlock_mux);
        _frozen_data = static_cast<char *> (vdata());
        _frozen_top_offset = top_offset();
        remalloc_free_mem();
        _have_frozen_buffer = true;
    }
    return asio::const_buffers_1(asio::buffer(_frozen_data + _frozen_sended_offset, _frozen_top_offset - _frozen_sended_offset));
}



} // namespace redis
