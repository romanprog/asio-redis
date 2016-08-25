#ifndef HNET_HPP
#define HNET_HPP
#include <string>

namespace hnet {

// Validate IP v4 address.
bool is_ip_v4(const std::string & ip);

// Validate IP v4 address.
bool is_fqdn(const std::string & name);

}
#endif // HNET_HPP
