#ifndef HSTRINGS_HPP
#define HSTRINGS_HPP

#include <string>
#include <vector>

namespace hstrings {

// Generate random char from alphanum[] set.
inline char rand_char();

// Very fast random string generator. Fill str by random chars. Perform resize of str. Return str ref.
std::string & rand_str(std::string & str, size_t lenth);
// Overloaad create new string object.
std::string rand_str(size_t lenth);

bool is_digit_only(const std::string & s);
bool is_alfa_only(const std::string & s);

// Split @str strint to vector @res (or construct and return new vector in case of @splitted function).
// Delimiters are not added to the parts.
// If @remove_empty == true - empty parts will be ignored.
std::vector<std::string> splitted(const std::string & str, char delim, bool remove_empty = false);
std::vector<std::string> & split(const std::string & str, std::vector<std::string> &res, char delim, bool remove_empty = false);

// Convert string to lower case.
std::string & to_lower(std::string & text);

// Get part number @pindex separated by @delim of @text .
// Numbering is zero base.
bool get_part(const std::string & text, std::string &result, char delim, size_t pindex = 0);

// Erase part number @pindex separated by @delim from @text and return result in @result string.
// Nearest delimiters will be deleted.
bool cut_part(const std::string & text, std::string &result, char delim, size_t pindex = 0);
bool cut_part(std::string & text, char delim, size_t pindex = 0);

// Same as @get_part and @cut_part, but reverse version. Numeration begin from the end of @text
bool get_part_reverse(const std::string & text, std::string &result, char delim, size_t pindex = 0);

bool cut_part_reverse(const std::string & text, std::string &result, char delim, size_t pindex = 0);

// Erase "isspace" characters from: 1) left, 2) rigth, 3) both sides of @text.
std::string & trim_l(std::string & text);
std::string & trim_r(std::string & text);
std::string & trim(std::string & text);

// Delete
}
#endif // HSTRINGS_HPP
