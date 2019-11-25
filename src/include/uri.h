#pragma once

#include <cpprest/details/basic_types.h>

/// @brief Decodes any %## encoding in the given string.
/// Plus symbols ('+') are decoded to a space character.
utility::string_t url_decode(const utility::string_t& encoded);