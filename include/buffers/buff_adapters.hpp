#ifndef BUFF_ADAPTERS_HPP
#define BUFF_ADAPTERS_HPP

#include <memory>

namespace redis {

namespace buff {

template <typename BaseBuffType>
class output_adapter
{
public:
    explicit output_adapter(BaseBuffType & buff_)
        :_base_buffer(buff_)
    {

    }

    // Return true if all contained data already sended (and confirmed).
    bool nothing_to_send() const
    {
        return _sended_offset >= _base_buffer.size();
    }

    // Confirm data part (@bytes_sended size) sending.
    void sending_report(size_t bytes_sended)
    {
        _sended_offset += bytes_sended;
    }

    // Pointer to begining of new (not sended) data
    const char * new_data() const
    {
        return static_cast<const char *>(_base_buffer.data() + _sended_offset);
    }

    const char * data() const
    {
        return static_cast<const char *>(_base_buffer.data());
    }

    // Size of new (not sended) data.
    size_t new_data_size() const
    {
        return _base_buffer.size() - _sended_offset;
    }


    size_t size() const
    {
        return _base_buffer.size();
    }

    BaseBuffType & get_ref()
    {
        return _base_buffer;
    }

    BaseBuffType & operator*()
    {
        return _base_buffer;
    }

private:

    BaseBuffType & _base_buffer;
    size_t _sended_offset {0};
};

/// Adapt string  for using as direct input buffer (read
/// redis respond directly into variable memory without copying)
class input_adapter
{
public:
    explicit input_adapter(std::string & buff_, size_t expected_sz_)
        :_base_buffer(buff_),
          expected_sz(expected_sz_)
    {
        // Reserve expected size + 25%
        _base_buffer.reserve(expected_sz + expected_sz/4);
    }

    const char * data() const
    {
        return static_cast<const char *>(_base_buffer.data());
    }


    size_t size() const
    {
        return _base_buffer.size();
    }

    void release(size_t sz)
    {
        _base_buffer.resize(sz);
    }

    bool accept(size_t sz)
    {
        _top_offset += sz;
    }

    std::string & get_ref()
    {
        return _base_buffer;
    }

    std::string & operator*()
    {
        return _base_buffer;
    }

private:

    std::string & _base_buffer;
    size_t _parsed_offset {0};
    size_t _top_offset {0};
    size_t expected_sz {0};

};




} // namespace buffers



} // namespace redis
#endif // BUFF_ADAPTERS_HPP
