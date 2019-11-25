#include "basic_controller.h"
#include "network_utils.h"
#include "common.h"
#include "uri.h"

namespace bs {
void BasicController::SetEndpoint(const std::string& value) {
  web::uri uri(value);
  web::uri_builder uri_builder;

  uri_builder.set_scheme(uri.scheme());

  if (uri.host() == "host_auto_ip4") {
    uri_builder.set_host(NetworkUtils::HostIP4());
  } else if (uri.host() == "host_auto_ip6") {
    uri_builder.set_host(NetworkUtils::HostIP6());
  } else {
    uri_builder.set_host(uri.host());
  }
  uri_builder.set_port(uri.port());
  uri_builder.set_path(uri.path());

  web::http::experimental::listener::http_listener_config conf;

  conf.set_ssl_context_callback([](boost::asio::ssl::context& ctx) {
    ctx.set_options(boost::asio::ssl::context::default_workarounds);

    if (app_config.kSSLKeyFile == "" || app_config.kSSLCertFile == "") {
      return;
    }

    // Password callback needs to be set before setting cert and key.
    ctx.set_password_callback([](std::size_t max_length, boost::asio::ssl::context::password_purpose purpose) {
      return app_config.kSSLContextPassword;
    });

    ctx.use_certificate_file(app_config.kSSLCertFile, boost::asio::ssl::context::pem);
    ctx.use_private_key_file(app_config.kSSLKeyFile, boost::asio::ssl::context::pem);

    if (app_config.kSSLChainFile == "") {
      return;
    }

    std::ifstream f(app_config.kSSLChainFile);
    if (f.good()) {
      ctx.use_certificate_chain_file(app_config.kSSLChainFile);
    }
  });

  _listener = web::http::experimental::listener::http_listener(uri_builder.to_uri(), conf);
}

std::string BasicController::Endpoint() const {
  return _listener.uri().to_string();
}

pplx::task<void> BasicController::Accept() {
  InitRESTHandlers();

  return _listener.open();
}

pplx::task<void> BasicController::Shutdown() {
  return _listener.close();
}

std::vector<utility::string_t> BasicController::RequestPath(const web::http::http_request & message) {
  auto relative_path = url_decode(message.relative_uri().path());

  return web::uri::split_path(relative_path);
}
} // namespace bs