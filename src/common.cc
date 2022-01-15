#include <chrono>
#include <iostream>
#include <iomanip>
#include <cpprest/version.h>

namespace bs {
const std::string Version() {
  return "1.2-1";
}

const std::string GetUserAgent() {
  std::stringstream s;
  s << "bambooslacking/" << Version() << " cpprest/" << CPPREST_VERSION;
  return s.str();
}

std::string GetCurrentTimestamp(const char* fmt, const long& offset) {
  auto tp = std::chrono::system_clock::now() + std::chrono::seconds(offset);
  auto in_time_t = std::chrono::system_clock::to_time_t(tp);
  auto tm = std::gmtime(&in_time_t);
  std::stringstream ss;

  ss << std::put_time(tm, fmt);

  return ss.str();
}

uint64_t strtotime(const std::string& s) {
  std::tm tt;
  std::stringstream ss;

  tt.tm_year = std::stoi(s.substr(0, 4)) - 1900;
  tt.tm_mon = std::stoi(s.substr(5, 2)) - 1;
  tt.tm_mday = std::stoi(s.substr(8, 2));
  tt.tm_hour = std::stoi(s.substr(11, 2));
  tt.tm_min = std::stoi(s.substr(14, 2));
  tt.tm_sec = std::stoi(s.substr(17, 2));
  tt.tm_gmtoff = 0;
  tt.tm_zone = "UTC";
  tt.tm_isdst = 0;

  ss << timegm(&tt);

  return std::stoi(ss.str());
}
} // namespace bs