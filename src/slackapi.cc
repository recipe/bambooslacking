#include "base64.h"
#include "slackapi.h"

using namespace web;
using namespace web::http;

namespace bs {
SlackApiClient::SlackApiClient(const std::string& kApiToken) : kApiToken(kApiToken) {}

/// @brief Helper function which reads a header comma separated value and returns split vector
/// @param result A result vector
/// @param headers A list of response headers
/// @param key A header key
static void GetHeaderScopes(
    std::vector<std::string>& result,
    const http_headers& headers,
    const utility::string_t& key
) {
  result.clear();

  if (const auto& it {headers.find(key)}; it != headers.end()) {
    boost::split(result, it->second, boost::is_any_of(","));
  }
}

json::value SlackApiClient::SendRequest(
    const method& mtd,
    const std::string& uri,
    const json::value& json_v
) {
  client::http_client client(kApiUrl);
  http_request req(mtd);

  req.headers().add(U("User-Agent"), GetUserAgent());
  req.headers().add(U("Accept-Charset"), U("utf-8"));
  // It uses bearer API token
  req.headers().add(U("Authorization"), U("Bearer " + kApiToken));

  if (mtd == methods::POST) {
    req.headers().add(U("Content-Type"), U("application/json; charset=utf-8"));

    if (json_v != kDefaultJsonValue) {
      req.set_body(json_v);
    }
  } else {
    req.headers().add(U("Accept"), U("application/json"));
  }

  req.set_request_uri(uri);

  http_response response = client.request(req).get();

  // Stores available OAuth scopes for the token
  GetHeaderScopes(oauth_scopes, response.headers(), U("X-OAuth-Scopes"));
  // Stores accepted OAuth scopes for the API request
  GetHeaderScopes(accepted_oauth_scopes, response.headers(), U("X-Accepted-OAuth-Scopes"));

  if (response.status_code() != 200) {
    throw std::runtime_error("Slack API error: " + response.to_string());
  }

  json::value result = response.extract_json().get();
  const bool no_ok = result.at(U("ok")).is_null();

  if (no_ok || !no_ok && result["ok"].as_bool() == false) {
    throw SlackApiError(
        "Slack API responded with error: "
        + (!result.at(U("error")).is_null() ? result["error"].as_string() : "unspecified")
    );
  }

  return result;
}

SlackUsersList SlackApiClient::UsersList(const BambooHrUsersList& accept) {
  int bamboohr_employee_id = 0;
  SlackUsersList users;

  std::string cursor {""};

  do {
    // Build request URI and start the request.
    uri_builder builder(U("/api/users.list"));
    builder.append_query(U("limit"), U("1000"));
    if (cursor != "") {
      builder.append_query(U("cursor"), cursor);
    }

    json::value json_response = SendRequest(methods::GET, builder.to_string());

    auto array = json_response.at(U("members")).as_array();

    for (int i = 0; i < array.size(); ++i) {
      auto& profileValue = array[i].at(U("profile"));
      const json::value& emailValue = profileValue[U("email")];

      // Bots and deleted users should be ignored
      if (emailValue.is_null()
          || true == array[i].at(U("deleted")).as_bool()
          ) {
        continue;
      }

      std::string email = emailValue.as_string();

      if (kDefaultAccept != accept) {
        // Checks if the user's email exists in the accept list
        auto iter = accept.find(email);
        if (iter == accept.end()) {
          // It does not exist
          continue;
        }

        // User has been found.
        // Use its employee id as an adjustment
        bamboohr_employee_id = iter->second;
      } else {
        bamboohr_employee_id = 0;
      }

      users[email] = {
          array[i].at(U("id")).as_string(),
          email,
          array[i].at(U("name")).as_string(),
          profileValue[U("real_name")].as_string(),
          profileValue[U("status_text")].as_string(),
          profileValue[U("status_emoji")].as_string(),
          profileValue[U("status_expiration")].as_integer(),
          profileValue[U("status_text_canonical")].as_string(),
          bamboohr_employee_id,
          array[i].at(U("tz_offset")).as_integer(),
          array[i].at(U("is_admin")).as_bool()
              || array[i].at(U("is_owner")).as_bool()
              || array[i].at(U("is_primary_owner")).as_bool()
      };
    }

    // Check if there is a next page
    cursor = !json_response.at(U("response_metadata")).is_null()
                 && !json_response.at(U("response_metadata")).at(U("next_cursor")).is_null()
             ? json_response.at(U("response_metadata")).at(U("next_cursor")).as_string()
             : "";
  } while (cursor != "");

  return users;
}

void SlackApiClient::UsersProfileSetStatus(
    const std::string& slack_id,
    const TimeOffProfile& to_profile,
    const uint64_t& status_expiration
) {
  json::value req;
  const std::string pr = U("profile");

  // Build request URI and start the request.
  uri_builder builder(U("/api/users.profile.set"));

  req[U("user")] = json::value::string(slack_id);
  req[pr][U("status_text")] = json::value::string(to_profile.text);
  req[pr][U("status_emoji")] = json::value::string(to_profile.emoji);
  req[pr][U("status_text_canonical")] = json::value::string(to_profile.text_canonical);
  req[pr][U("status_expiration")] = json::value::number(status_expiration);

  SendRequest(methods::POST, builder.to_string(), req);
}

json::value SlackApiClient::UsersInfo(const std::string& user_id) {
  uri_builder builder(U("/api/users.info"));
  builder.append_query(U("user"), user_id);

  return SendRequest(methods::GET, builder.to_string());
}

json::value SlackApiClient::OauthAccess(
    const std::string& client_id,
    const std::string& client_secret,
    const std::string& code
) {
  client::http_client client(kApiUrl);
  http_request req(methods::POST);

  req.headers().add(U("User-Agent"), GetUserAgent());

  uri_builder builder(U("/api/oauth.access"));
  req.set_request_uri(builder.to_uri());

  req.set_body(
      "code=" + uri::encode_data_string(code) +
      "&client_id=" + uri::encode_data_string(client_id) +
      "&client_secret=" + uri::encode_data_string(client_secret),
      "application/x-www-form-urlencoded"
  );

  http_response response = client.request(req).get();

  if (response.status_code() != 200) {
    throw std::runtime_error(
        "SlackApiClient::OauthAccess request responded with error: " + response.to_string()
    );
  }

  return response.extract_json(true).get();
}

bool SlackApiClient::ApiTest() {
  uri_builder builder(U("/api/api.test"));
  json::value value;
  value[U("foo")] = json::value::string(U("bar"));

  json::value result = SendRequest(methods::POST, builder.to_string(), value);

  return !result.at(U("ok")).is_null() && result[U("ok")].as_bool() ? true : false;
}

std::vector<std::string> SlackApiClient::GetScopes() {
  if (oauth_scopes.size() == 0) {
    // There has not been any request yet.
    ApiTest();
  }

  return oauth_scopes;
}

std::vector<std::string> SlackApiClient::GetAcceptedScopes() {
  return accepted_oauth_scopes;
}
} //namespace bs