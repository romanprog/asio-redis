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

bool output_buff::nothing_to_send()
{
    return top_offset() <= _sended_offset.load();
}

void output_buff::sending_report(size_t bytes_sended)
{
    _sended_offset += bytes_sended;
    manage_mem();
}

const char *output_buff::new_data()
{
    return data() + _sended_offset.load();
}

size_t output_buff::new_data_size()
{
    return top_offset() - _sended_offset.load();
}

void output_buff::add_query(const std::string &query, bool plus_rn)
{

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
    // manage_mem();
    if ((top_offset() + need_write) >= max_buff_size)
        return true;

    return false;
}

void output_buff::manage_mem()
{
    // Lock mutex before memory changes.
    // Thish mutex must be locked in pipeline processor before "async_send", and unlock in the beginning of confirmation callback.
    // This prevents reading from memory while managing.
    std::lock_guard<std::mutex> _mem_lock(*_realloc_mux);

    if (nothing_to_send()) {
        reset(true);
        _sended_offset.store(0);
        return;
    }

    if (size_reserved() > size_filled() * 2)
        return;

    if ((top_offset() - new_data_size()) < new_data_size())
        return;

    memcpy(vdata(), new_data(), new_data_size());
    change_data_top(new_data_size());
    _sended_offset.store(0);
}


} // namespace redis
