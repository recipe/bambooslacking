#pragma once

#include <cpprest/http_msg.h>

namespace bs {
/// @brief abstract controller class
class Controller {
 public:
  /// @brief Handles GET requests
  /// @param message HTTP request message
  virtual void HandleGet(web::http::http_request message) = 0;
  /// @brief Handles POST requests
  /// @param message HTTP request message
  virtual void HandlePost(web::http::http_request message) = 0;
};
} // namespace bs