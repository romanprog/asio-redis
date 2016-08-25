#include <vector>
#include <arpa/inet.h>
#include <algorithm>

#include "../include/utils/h_net.hpp"
#include "../include/utils/h_strings.hpp"

namespace hnet {

bool is_ip_v4(const std::string &ip)
{
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr))!=0;
}

bool is_fqdn(const std::string &name)
{
    if (name.empty())
        return false;

    // Search invalid symbols.
    if (std::find_if(name.begin(), name.end(), [](char c) { return (!std::isalnum(c) && c != '-' && c != '.' ) ; }) != name.end())
        return false;

    std::vector<std::string> vec_tmp;
    hstrings::split(name, vec_tmp, '.');

    if (vec_tmp.size() < 2)
        return false;

    for (std::string & s : vec_tmp)
        if (s.empty())
            return false;

    return true;
}

}
