#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <regex>

namespace bs {
/// @brief Templates directory
inline const std::string kTemplatesDIR {"/opt/bambooslacking/templates/"};
/// @brief PID file
constexpr const char* kPidFile {"/var/run/bambooslacking.pid"};
/// @brief Path to configuration file
inline const std::string kConfigFile {"/etc/bambooslacking/config.json"};
/// @brief Path to log file
inline const std::string kLogFile {"/var/log/bambooslacking.log"};
/// @brief Application database path
inline const std::string kDbName {"/opt/bambooslacking/db/bsdb"};
/// @brief The name of the command
inline const std::string kCommandName {"whoisout"};
/// @brief An alphanumeric regular expression pattern
inline const std::regex kRegexAlphanum {"[a-z0-9]+", std::regex::icase};
/// @brief A word regular expression pattern
inline const std::regex kRegexWord {"[a-z][a-z0-9_]+", std::regex::icase};

// Configuration options keys
inline const std::string kCfgSlackClientID {"slack_client_id"};
inline const std::string kCfgSlackClientSecret {"slack_client_secret"};
inline const std::string kCfgSlackSigningSecret {"slack_signing_secret"};
inline const std::string kCfgCryptokey {"cryptokey"};
inline const std::string kCfgServerEndpoint {"server_endpoint"};
inline const std::string kCfgSSLCert {"ssl_cert_pem"};
inline const std::string kCfgSSLKey {"ssl_key_pem"};
inline const std::string kCfgSSLFullchain {"ssl_fullchain_pem"};
inline const std::string kCfgSSLContextPassword {"ssl_context_password"};

/// @brief text/html; charset=utf-8 string that is used in ContentType header
inline const std::string kContentTypeTextHTMLCharsetUTF8 {"text/html; charset=utf-8"};

/// @brief Describes user profile
struct UserProfile {
  /// @brief Slack user identifier
  std::string slack_id;
  /// @brief Slack user email
  std::string email;
  /// @brief Slack username
  std::string name;
  /// @brief Slack user real name
  std::string real_name;
  /// @brief Slack user's profile status text
  std::string status_text;
  /// @brief Slack user's profile status emoji
  std::string status_emoji;
  /// @brief Slack user's profile status expiration (unix)
  long int status_expiration;
  /// @brief Slack user's profile status text canonical
  std::string status_text_canonical;
  /// @brief BambooHR employeeId
  int bamboohr_employee_id;
  /// @brief time zone offset (GMT)
  int tz_offset;
  /// @brief whether user is admin or owner
  bool is_privileged;
};

// email, employee_id
typedef std::map<std::string, int> BambooHrUsersList;
// employee_id => [date => time-off name[]]
typedef std::map<int, std::map<std::string, std::vector<std::string>>> BambooHrTimeOffList;
/// email, user_profile
typedef std::map<std::string, UserProfile> SlackUsersList;

struct Config {
  /// @brief Slack application client ID issued on app creation
  std::string kSlackClientID;
  /// @brief Slack application client secret issued on app creation
  std::string kSlackClientSecret;
  /// @brief Confirm that each request comes from Slack by verifying its unique signature.
  std::string kSlackSigningSecret;
  /// @brief A key which is used for an encryption
  std::string kCryptokey;
  /// @brief Application server endpoint
  std::string kServerEndpoint;
  /// @brief SSL cert.pem file
  std::string kSSLCertFile;
  /// @brief SSL key.pem file
  std::string kSSLKeyFile;
  /// @brief SSL fullchain.pem file
  std::string kSSLChainFile;
  /// @brief Context password for SSL certificate
  std::string kSSLContextPassword;
};

/// @brief Application config is initializes once on load
inline Config app_config;

/// @brief Gets an application version
const std::string Version();

/// @brief Gets user-agent version that is used in API client headers
const std::string GetUserAgent();

/// @brief Gets current datetime as specified by format (in UTC timezone)
/// @param fmt format of the result timestamp value
/// @param offset offset in seconds
std::string GetCurrentTimestamp(const char* fmt, const long& offset);

/// @brief Gets unix time from the "YYYY-MM-DD HH:MM:SS" timestamp.
/// @param s a timestamp is expected to be provided in the UTC time zone.
uint64_t strtotime(const std::string& s);
} // namespace bs