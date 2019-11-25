#include <cpprest/base_uri.h>
#include "uri.h"

/// @brief Helper function to convert a hex character digit to a decimal character value.
/// @throws uri_exception if not a valid hex digit.
static int hex_char_digit_to_decimal_char(int hex) {
  int decimal;

  if (hex >= '0' && hex <= '9') {
    decimal = hex - '0';
  } else if (hex >= 'A' && hex <= 'F') {
    decimal = 10 + (hex - 'A');
  } else if (hex >= 'a' && hex <= 'f') {
    decimal = 10 + (hex - 'a');
  } else {
    throw web::uri_exception("Invalid hexadecimal digit");
  }

  return decimal;
}

template<class String>
static std::string decode_template(const String& encoded) {
  std::string raw;
  for (auto iter = encoded.begin(); iter != encoded.end(); ++iter) {
    if (*iter == '%') {
      if (++iter == encoded.end()) {
        throw web::uri_exception("Invalid URI string, two hexadecimal digits must follow '%'");
      }
      int decimal_value = hex_char_digit_to_decimal_char(static_cast<int>(*iter)) << 4;
      if (++iter == encoded.end()) {
        throw web::uri_exception("Invalid URI string, two hexadecimal digits must follow '%'");
      }
      decimal_value += hex_char_digit_to_decimal_char(static_cast<int>(*iter));

      raw.push_back(static_cast<char>(decimal_value));
    } else if (*iter > 127 || *iter < 0) {
      throw web::uri_exception("Invalid encoded URI string, must be entirely ascii");
    } else if (*iter == '+') {
      raw.push_back(' ');
    } else {
      // encoded string has to be ASCII.
      raw.push_back(static_cast<char>(*iter));
    }
  }
  return raw;
}

// We have to override uri::decode method as it does not decode "+" as a space character
utility::string_t url_decode(const utility::string_t& encoded) {
  return utility::conversions::to_string_t(decode_template(encoded));
}