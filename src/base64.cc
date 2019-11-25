#include <cassert>
#include <stdexcept>
#include "base64.h"

std::string base64_encode(const std::string& bindata) {
  using std::string;
  using std::numeric_limits;

  if (bindata.size() > (numeric_limits<string::size_type>::max() / 4u) * 3u) {
    throw std::length_error("Converting too large a string to base64.");
  }

  const std::size_t binlen = bindata.size();
  string retval((((binlen + 2) / 3) * 4), '=');
  std::size_t outpos = 0;
  int bits_collected = 0;
  unsigned int accumulator = 0;
  const string::const_iterator binend = bindata.end();

  for (string::const_iterator i = bindata.begin(); i != binend; ++i) {
    accumulator = (accumulator << 8) | (*i & 0xffu);
    bits_collected += 8;
    while (bits_collected >= 6) {
      bits_collected -= 6;
      retval[outpos++] = b64_table[(accumulator >> bits_collected) & 0x3fu];
    }
  }
  if (bits_collected > 0) {
    assert(bits_collected < 6);
    accumulator <<= 6 - bits_collected;
    retval[outpos++] = b64_table[accumulator & 0x3fu];
  }
  assert(outpos >= (retval.size() - 2));
  assert(outpos <= retval.size());

  return retval;
}

std::string base64_decode(const std::string& ascdata) {
  using std::string;

  string retval;
  const string::const_iterator last = ascdata.end();
  int bits_collected = 0;
  unsigned int accumulator = 0;

  for (string::const_iterator i = ascdata.begin(); i != last; ++i) {
    const int c = *i;
    if (std::isspace(c) || c == '=') {
      continue;
    }
    if ((c > 127) || (c < 0) || (reverse_table[c] > 63)) {
      throw std::invalid_argument("It contains illegal characters in a base64 encoded string.");
    }
    accumulator = (accumulator << 6) | reverse_table[c];
    bits_collected += 6;
    if (bits_collected >= 8) {
      bits_collected -= 8;
      retval += static_cast<char>((accumulator >> bits_collected) & 0xffu);
    }
  }

  return retval;
}
