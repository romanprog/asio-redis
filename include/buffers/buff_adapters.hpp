#ifndef BUFF_ADAPTERS_HPP
#define BUFF_ADAPTERS_HPP

#include <memory>

namespace redis {

namespace buff {

template <typename BaseBuffType>
class output_adapter
{
public:
    template <typename ...Args>
    explicit output_adapter(Args... args)
        :_base_buffer(std::make_shared<BaseBuffType>(std::forward<Args>(args)...))
    {

    }
    // Return true if all contained data already sended (and confirmed).
    bool nothing_to_send() const
    {
        return _sended_offset >= _base_buffer->size();
    }

    // Confirm data part (@bytes_sended size) sending.
    void sending_report(size_t bytes_sended)
    {
        _sended_offset += bytes_sended;
    }

    // Pointer to begining of new (not sended) data
    const char * new_data() const
    {
        return static_cast<const char *>(_base_buffer->data() + _sended_offset);
    }

    const char * data() const
    {
        return static_cast<const char *>(_base_buffer->data());
    }

    // Size of new (not sended) data.
    size_t new_data_size() const
    {
        return _base_buffer->size() - _sended_offset;
    }


    size_t size() const
    {
        return _base_buffer->size();
    }

    BaseBuffType & get_ref()
    {
        return *_base_buffer.get();
    }

private:

    std::shared_ptr<BaseBuffType> _base_buffer;
    size_t _sended_offset {0};
};


} // namespace buffers



} // namespace redis
#endif // BUFF_ADAPTERS_HPP
