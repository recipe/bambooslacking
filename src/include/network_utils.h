#pragma once

#include <string>
#include <boost/asio.hpp>

namespace bs {
class NetworkUtils {
 private:
  static boost::asio::ip::tcp::resolver::iterator QueryHostInetInfo();
  static std::string HostIP(unsigned short family);
 public:
  // gets the host IP4 string formatted
  static std::string HostIP4() {
    return HostIP(AF_INET);
  }
  // gets the host IP6 string formatted
  static std::string HostIP6() {
    return HostIP(AF_INET6);
  }
  static std::string HostName() {
    return boost::asio::ip::host_name();
  }
};
} // namespace bs