#include "../include/buffers/buff_abstract.hpp"

#include <stdexcept>


buff_abstract::buff_abstract()
    : _reserved(calculate_mem(_basic_block_size))
{
    // In this case will be called _mem_calc() of base class, as need to allocate first memory block.
    // Override _mem_calc() in derived classes to make own memory managment in release() method
    // and other memory managers of derived classes.
    _cdata = static_cast<char *>(malloc(_reserved));
}

buff_abstract::~buff_abstract()
{
    // Free main data block.
    free(_cdata);
}

const char *buff_abstract::data() const
{
    return static_cast<const char * const> (_cdata);
}

void *buff_abstract::data_top()
{
    // No free space.
    if (!size_avail())
        return nullptr;

    // Pointer to begin of free space.
    return _cdata + _top_offset;
}

void *buff_abstract::vdata()
{
    return _cdata;
}

bool buff_abstract::accept(size_t bytes_readed)
{
    if (bytes_readed > size_avail())
        return false;

    _top_offset += bytes_readed;

    when_new_data_acc(bytes_readed);
    return true;
}

void buff_abstract::release(size_t size)
{
    if (_size > 80000000)
        throw std::out_of_range("Buffer overflow. Max - 80Mb");

    if (size < size_avail())
        return;

    size_t _reserved_free = _reserved - _top_offset;

    if (_reserved_free <= size) {
        _reserved = calculate_mem(size);
        _cdata = static_cast<char *>(realloc(_cdata, _reserved));
    }
    _size = _top_offset + size;

    if (_size == _top_offset) {
        return;
    }

}

size_t buff_abstract::size_filled() const
{
    return _top_offset;
}


void buff_abstract::reset(bool soft_reset)
{
    _top_offset = _size = 0;

    if (!soft_reset) {
        _reserved = calculate_mem(_basic_block_size);
        _cdata = static_cast<char *>(realloc(_cdata, _reserved));

    }
    when_reseted();
}

size_t buff_abstract::size() const
{
    return _size;
}

size_t buff_abstract::size_avail() const
{
    return _size - _top_offset;
}

size_t buff_abstract::size_reserved() const
{
    return _reserved;
}

void buff_abstract::operator <<(const char *str)
{
    size_t sz = std::strlen(str);
    release(sz);
    memcpy(data_top(), str, sz);
    accept(sz);
}

void buff_abstract::operator <<(const std::string &str)
{
    release(str.size());
    memcpy(data_top(), str.data(), str.size());
    accept(str.size());
}

size_t buff_abstract::calculate_mem(size_t block_size)
{
    size_t reserve_bl_count {2};
    // Base mem reserv calculate. 1 block for data needeng + 1 free block.
    return ((_top_offset + size_filled()) / block_size + reserve_bl_count) * block_size;
}

void buff_abstract::when_new_data_acc(size_t bytes_readed)
{

}

void buff_abstract::when_reseted()
{

}

size_t buff_abstract::top_offset() const
{
    return _top_offset;
}

void buff_abstract::change_data_top(size_t new_data_top)
{
    _top_offset = new_data_top;
}

void buff_abstract::reset_size()
{
    _size = 0;
}
