#pragma once

#include <string>
#include <cpprest/http_listener.h>

namespace bs {
/// @brief Basic controller for the application REST service
class BasicController {
 protected:
  /// @brief A HTTP listener
  web::http::experimental::listener::http_listener _listener;

 public:
  BasicController() {};
  ~BasicController() {};

  /// @brief Sets application server Endpoint URL
  /// @param value Endpoint URL (https://server.host:port) where application will listen incoming requests
  void SetEndpoint(const std::string& value);

  /// @brief Gets Endpoint URL
  std::string Endpoint() const;

  /// @brief Starts accepting incoming requests and initializes all necessary handlers
  /// @returns A task that will be completed once this listener is actually opened, accepting requests.
  pplx::task<void> Accept();

  /// @brief Stops accepting incoming requests
  /// @returns A task that will be completed once this listener is actually closed, no longer accepting requests.
  pplx::task<void> Shutdown();

  /// @brief Abstract method to initialize REST handlers
  virtual void InitRESTHandlers() {};

  /// @brief Function parses URL path from the incoming request message and splits it into the chunks
  /// @param message An HTTP request message
  /// @returns URL path chunks
  std::vector<utility::string_t> RequestPath(const web::http::http_request& message);
};
} // namespace bs