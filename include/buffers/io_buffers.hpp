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
class output_buff : public buff_abstract
{
public:
    output_buff();
    // Return true if all contained data already sended (and confirmed).
    bool nothing_to_send();
    // Confirm data part (@bytes_sended size) sending.
    void sending_report(size_t bytes_sended);
    // Pointer to begining of new (not sended) data
    const char * new_data();
    // Size of new (not sended) data.
    size_t new_data_size();
    // Add new query tu buffer line.
    bool add_query(const std::string &query);

private:
    // Memory management: cleaning, fast reset, data transfer on free sites
    // to avoid the appearance of a new memory.
    void manage_mem();
    size_t _sended_offset {0};

};


} //namespace redis
#endif // RESPBUFFER_HPP




