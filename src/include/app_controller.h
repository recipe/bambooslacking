#pragma once

#include "controller.h"
#include "basic_controller.h"
#include "common.h"

namespace bs {
/// @brief Slack command usage message
inline std::string kCommandUsage {
"These are available " + kCommandName + " commands:\n"
"`/" + kCommandName + "` Get information about teammates who are out today.\n"
"`/" + kCommandName + " install <org name> <api secret>` Install BambooHR API token for your team. "
"`<org name>` is the name of your organization as it is used in the BambooHR API."
};

/// @brief Application service controller
class AppController : public BasicController, Controller {
 public:
  AppController() : BasicController() {}
  ~AppController() {}

  void HandleGet(web::http::http_request message) override;
  void HandlePost(web::http::http_request message) override;
  void InitRESTHandlers() override;

 private:
  /// @brief Gets Not Implemented response
  static web::json::value responseNotImpl(const web::http::method& method);
};
} // namespace bs