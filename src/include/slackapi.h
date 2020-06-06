#pragma once

#include <stdexcept>
#include <boost/algorithm/string.hpp>
#include <cpprest/http_client.h>
#include "common.h"

namespace bs {
/// @brief Default value for the json_v function optional argument
static inline const web::json::value kDefaultJsonValue;
/// @brief Default value for the accept function optional argument
static inline const BambooHrUsersList kDefaultAccept;

/// @brief Describes time off profile
struct TimeOffProfile {
  /// @brief Slack user's profile status text
  std::string text;
  /// @brief Slack user's profile status emoji
  std::string emoji;
  /// @brief Slack user's profile status text canonical
  std::string text_canonical;
};

/// TypeName, time_off_type
typedef std::map<int, TimeOffProfile> TimeOffTypesList;

/// @brief Declares internally supported time off types
struct TimeOff {
  /// @brief Collection of time-off types
  /// The less int value of the time-off type the highest priority it has.
  enum Type : uint8_t {
    OVERTIME_WORK, // has the highest priority
    VACATION,
    DAY_OFF,
    SICK,
    REMOTE_WORK,
    BUSINESS_TRIP,
    BEREAVEMENT
  };

  /// @brief Mapping for the human readable names of the time offs that come from bambooHR
  static inline const std::map<std::string, Type> NAMES = {
      {"Overtime Work", Type::OVERTIME_WORK},
      {"Vacation", Type::VACATION},
      {"Unpaid Day Off", Type::DAY_OFF},
      {"Sick", Type::SICK},
      {"Remote Work", Type::REMOTE_WORK},
      {"Business trip", Type::BUSINESS_TRIP},
      {"Bereavement", Type::BEREAVEMENT}
  };

  /// @brief Time off types profiles.
  /// It defines which emoji and status text will appear in Slack
  static inline const TimeOffTypesList TYPES = {
      {
          TimeOff::Type::OVERTIME_WORK,
          {
              "Overtime Work",
              ":bee:",
              "Overtime Work"
          }
      },
      {
          TimeOff::Type::VACATION,
          {
              "On holiday",
              ":palm_tree:",
              "Vacationing"
          }
      },
      {
          TimeOff::Type::DAY_OFF,
          {
              "Day off",
              ":family:",
              "Day off"
          }
      },
      {
          TimeOff::Type::SICK,
          {
              "Out sick",
              ":face_with_thermometer:",
              "Out sick"
          }
      },
      {
          TimeOff::Type::REMOTE_WORK,
          {
              "Working remotely",
              ":house_with_garden:",
              "Working remotely"
          }
      },
      {
          TimeOff::Type::BUSINESS_TRIP,
          {
              "Business trip",
              ":airplane:",
              "Business trip"
          }
      },
      {
          TimeOff::Type::BEREAVEMENT,
          {
              "Bereavement leave",
              ":pray:",
              "Bereavement leave"
          }
      }
  };
};

/// @brief Slack API client
class SlackApiClient {
 public:
  /// @brief Slack API base URL
  static inline const std::string kApiUrl = U("https://slack.com/");

  SlackApiClient(const std::string& kApiToken);

  /// @brief Sends API request
  /// @param mtd HTTP method
  /// @param uri Request URI
  /// @param json_v JSON is being used in POST HTTP requests
  web::json::value SendRequest(
      const web::http::method& mtd,
      const std::string& uri,
      const web::json::value& json_v = kDefaultJsonValue
  );

  /// @brief Gets the list of the Users.
  /// If the access_token does not have permission to view user email it will return empty list.
  /// @param accept the list of the the user's emails to accept for the response
  SlackUsersList UsersList(const BambooHrUsersList& accept = kDefaultAccept);

  /// @brief Sets users profile status
  /// @param slack_id Slack user identifier
  /// @param to_profile time off profile to set
  /// @param status_expiration status expiration to set
  void UsersProfileSetStatus(
      const std::string& slack_id,
      const TimeOffProfile& to_profile,
      const uint64_t& status_expiration
  );

  /// @brief Gets information about user
  /// @param user_id An user identifier
  /// @returns Detailed information about requested user as JSON object
  web::json::value UsersInfo(const std::string& user_id);

  /// @brief Exchanges a temporary OAuth verifier code for an access token
  /// @param client_id A Slack Client ID
  /// @param client_secret A Slack Client Secret
  /// @param code An exchange code returned via OAuth callback
  /// @returns JSON response
  web::json::value OauthAccess(
      const std::string& client_id,
      const std::string& client_secret,
      const std::string& code
  );

  /// @brief Invokes Slack api.test method to check token and retrieve available OAuth scopes
  /// @returns TRUE on success
  bool ApiTest();

  /// @brief Gets a list for available OAuth scopes for the Token
  std::vector<std::string> GetScopes();

  /// @brief Gets a list of accepted OAuth scopes for the last API request
  std::vector<std::string> GetAcceptedScopes();
 private:
  /// @brief API token to sign requests
  const std::string kApiToken;

  /// @brief Contains oauth scopes for the API token
  std::vector<std::string> oauth_scopes;

  /// @brief Contains the list of accepted OAuth scopes for the last API request
  std::vector<std::string> accepted_oauth_scopes;
};

/// @brief Slack API error responses (when "ok" is false) will cause this exception
class SlackApiError : public std::runtime_error {
  /// @brief Error types
 public:
  // we are using original constructor
  using std::runtime_error::runtime_error;

  enum Type : uint8_t {
    USER_NOT_FOUND,
    USER_NOT_VISIBLE,
    NOT_AUTHED,
    INVALID_AUTH,
    ACCOUNT_INACTIVE,
    TOKEN_REVOKED,
    NO_PERMISSION,
    ORG_LOGIN_REQUIRED,
    EKM_ACCESS_DENIED,
    MISSING_SCOPE,
    INVALID_ARGUMENTS,
    INVALID_ARG_NAME,
    INVALID_CHARSET,
    INVALID_FORM_DATA,
    INVALID_POST_TYPE,
    MISSING_POST_TYPE,
    TEAM_ADDED_TO_ORG,
    REQUEST_TIMEOUT,
    FATAL_ERROR
  };

  /// @brief Mapping of types to its names
  static inline const std::map<Type, std::string> kName = {
      {USER_NOT_FOUND, U("user_not_found")},
      {USER_NOT_VISIBLE, U("user_not_visible")},
      {NOT_AUTHED, U("not_authed")},
      {INVALID_AUTH, U("invalid_auth")},
      {ACCOUNT_INACTIVE, U("account_inactive")},
      {TOKEN_REVOKED, U("token_revoked")},
      {NO_PERMISSION, U("no_permission")},
      {ORG_LOGIN_REQUIRED, U("org_login_required")},
      {EKM_ACCESS_DENIED, U("ekm_access_denied")},
      {MISSING_SCOPE, U("missing_scope")},
      {INVALID_ARGUMENTS, U("invalid_arguments")},
      {INVALID_ARG_NAME, U("invalid_arg_name")},
      {INVALID_CHARSET, U("invalid_charset")},
      {INVALID_FORM_DATA, U("invalid_form_data")},
      {INVALID_POST_TYPE, U("invalid_post_type")},
      {MISSING_POST_TYPE, U("missing_post_type")},
      {TEAM_ADDED_TO_ORG, U("team_added_to_org")},
      {REQUEST_TIMEOUT, U("request_timeout")},
      {FATAL_ERROR, U("fatal_error")}
  };

  /// @brief Check if the error of provided type
  /// @param t A type to check
  /// @returns TRUE of the error is the specified type
  bool IsError(const Type t) {
    return what() == kName.at(t);
  }

  /// @brief Check whether error is related to invalid or revoked token,
  /// or token that does not have minimum permissions needed to perform operations.
  bool IsInvalidTokenError() {
    auto w = what();
    return IsError(NOT_AUTHED)
        || IsError(TOKEN_REVOKED)
        || IsError(INVALID_AUTH)
        || IsError(NO_PERMISSION)
        || IsError(MISSING_SCOPE)
        || IsError(ACCOUNT_INACTIVE);
  }
};
} // namespace bs