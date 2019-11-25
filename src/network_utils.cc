#include "network_utils.h"

namespace bs {
boost::asio::ip::tcp::resolver::iterator NetworkUtils::QueryHostInetInfo() {
  using namespace boost::asio;
  io_service ios;
  ip::tcp::resolver resolver(ios);
  ip::tcp::resolver::query query(ip::host_name(), "");

  return resolver.resolve(query);
}

std::string NetworkUtils::HostIP(unsigned short family) {
  using namespace boost::asio::ip;
  auto host_inet_info = QueryHostInetInfo();
  tcp::resolver::iterator end;

  while (host_inet_info != end) {
    tcp::endpoint ep = *host_inet_info++;
    sockaddr sa = *ep.data();
    if (sa.sa_family == family) {
      return ep.address().to_string();
    }
  }

  return nullptr;
}
} // namespace bs