#pragma once

#include <stdexcept>
#include <cpprest/http_client.h>
#include "common.h"

namespace bs {
/// @brief BambooHR API client
class BambooHrApiClient {
 public:
  /// @brief BambooHR API client base URL
  static inline const std::string kApiUrl = U("https://api.bamboohr.com/");

  /// @brief Constructor
  /// @param kApiToken BambooHR API token
  /// @param kOrgName BambooHR organization name is used as a part of API URL
  BambooHrApiClient(const std::string& kApiToken, const std::string& kOrgName);

  /// @brief Sends API request
  /// @param mtd HTTP method
  /// @param uri request URI
  web::json::value SendRequest(const web::http::method& mtd, const std::string& uri);

  /// @brief Gets the list of the Users
  BambooHrUsersList UsersList();

  /// @brief Gets the list of the today's time-offs of the users that were approved for today.
  /// @param start_date a start date of the period
  /// @param end_date an end date of the period
  BambooHrTimeOffList WhoIsOut(const std::string& start_date, const std::string& end_date);

 private:
  ///@brief BambooHR API token to sing requests
  const std::string kApiToken;
  ///@brief BambooHR organization name is the part of the API URL
  const std::string kOrgName;
};

/// @brief BambooHR API error
class BambooHrApiError : public std::exception {
 public:
  /// @param code HTTP error code
  BambooHrApiError(const uint16_t& code) : std::exception(), code{code}, m_what{""} {}

  /// @param code HTTP error code
  /// @param m An error message
  BambooHrApiError(const uint16_t& code, const std::string& m) : std::exception(), code{code}, m_what{m} {}

  ~BambooHrApiError() {}

  const char* what() const noexcept {
    utf8stringstream ss;

    ss << "Error code: " << code;

    if (m_what != "") {
      ss << " " << m_what;
    }

    return ss.str().c_str();
  }
 private:
  /// @brief error message
  std::string m_what;
  /// @brief error code
  uint16_t code;
};
} // namespace bs