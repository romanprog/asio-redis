#ifndef RESPBUFFER_HPP
#define RESPBUFFER_HPP

#include "buff_abstract.hpp"
#include "../types.hpp"

#include <string>

namespace redis {


class input_buff : public buff_abstract
{
public:
    input_buff();


private:
    friend class resp_proto;
    bool _incompl_arr {false};
    size_t _unparsed_offset {0};
    bool _comlated {false};

    virtual void when_reseted();
    size_t unparsed_size();
    const char * unparsed_data();
    void manage_mem();

};

/// /////////////////  Output buffer //////////////////////
class output_buff : private buff_abstract
{
public:
    output_buff();
    ~output_buff();
    // Return true if all contained data already sended (and confirmed).
    bool nothing_to_send();
    // Confirm data part (@bytes_sended size) sending.
    void sending_report(size_t bytes_sended);
    // Add new query tu buffer line. If %plus_rn is true - add '\r\n' at the end of %query.
    void add_query(const std::string &query, bool plus_rn = false);
    // Check @max_buff_size overflow whith new query @need_write size.
    bool check_overflow(size_t need_write);

    void reset();

    asio::const_buffers_1 get_buffer();


private:
    // Memory management: cleaning, fast reset, data transfer on free sites
    // to avoid the appearance of a new memory.
    void manage_mem();
    std::atomic<size_t> _frozen_sended_offset {0};
    std::mutex _memlock_mux;
    char * _frozen_data {nullptr};
    bool _have_frozen_buffer {false};
    size_t _frozen_top_offset {0};
};


} //namespace redis
#endif // RESPBUFFER_HPP
