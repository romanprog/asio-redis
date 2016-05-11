#include <string>
#include <cstring>
#include <vector>
#include <iostream>

#include "../include/types.hpp"

namespace redis {

size_t ins_query_prefix(std::string &query_, unsigned pcount_)
{
    // Real prefix length.
    unsigned pref_len {1};
    for (int t = 10; pcount_/t > 0; t*=10, ++pref_len);
    pref_len += 3; // 3 bytes for "*\r\n"

    // Create prefix (*unsigned_query_parts_count\r\n. See RESP protocol) and copy it into reserved memory.
    sprintf(&query_[query_pref_max_size - pref_len], "*%d\r\n", pcount_);
    query_[query_pref_max_size] = '$';

    // Erase extra bytes. Erase in the string begining (trim) - dont do mem relocation.
    query_.erase(query_.begin(), query_.begin() + query_pref_max_size - pref_len);

    return query_pref_max_size - pref_len;
}

void add_query_part(std::string &query_, const char *part_)
{
    size_t sz = strlen(part_);
    auto old_sz = query_.size();
    query_.resize(old_sz + query_pref_max_size + sz + 2);
    size_t added_size = snprintf(&query_[old_sz], query_pref_max_size + sz + 2, "$%d\r\n%s\r\n", static_cast<unsigned>(sz), part_);
    query_.erase(old_sz + added_size);

}

std::vector<asio::const_buffer> build_multibuffer(const std::string &query_, const DBuffsPosList &ext_buffs_list_)
{
    std::vector<asio::const_buffer> res;
    size_t prev_pos {0};
    for (auto & pos : ext_buffs_list_)
    {
//        std::cout << std::string(query_.data() + prev_pos, pos.first - prev_pos);

        res.push_back(asio::buffer(query_.data() + prev_pos, pos.first - prev_pos));
        res.push_back(pos.second);
        prev_pos = pos.first;
    }
//    std::cout << std::string(query_.data() + prev_pos, query_.size() - prev_pos);
    res.push_back(asio::buffer(query_.data() + prev_pos, query_.size() - prev_pos));
    return res;
}

}
