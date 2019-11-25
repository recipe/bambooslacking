#include <regex>
#include <cpprest/filestream.h>
#include <bamboohrapi.h>
#include "common.h"
#include "encryption.h"
#include "db.h"
#include "uri.h"
#include "slackapi.h"
#include "easylogging++.h"
#include "app_controller.h"

using namespace web;
using namespace web::http;

namespace bs {
void AppController::InitRESTHandlers() {
  _listener.support(
      methods::GET,
      std::bind(&AppController::HandleGet, this, std::placeholders::_1)
  );
  _listener.support(
      methods::POST,
      std::bind(&AppController::HandlePost, this, std::placeholders::_1)
  );
}

template<typename _CharType>
pplx::task<concurrency::streams::streambuf<_CharType>> OPEN_R(const std::string& file)
{
  return concurrency::streams::file_buffer<_CharType>::open(kTemplatesDIR + file, std::ios_base::in);
}

/// @brief Create a reply for a Slack command request
/// @param url Slack command Response URL
/// @param message Reply message
static void PostToResponseURL(const std::string& url, const std::string& message) {
  client::http_client client(url);
  http_request req(methods::POST);

  req.headers().add(U("Content-Type"), U("application/json; charset=utf-8"));

  if (!message.empty() && message[0] != '{') {
    json::value msg;
    msg[U("text")] = json::value::string(message);
    req.set_body(msg);
  } else {
    req.set_body(message);
  }

  http_response response = client.request(req).get();

  if (response.status_code() != 200) {
    throw std::runtime_error("Unable to POST to response URL: " + response.to_string());
  }
}

/// @brief Process install command
/// @param response_url Slack command Response URL
/// @param trigger_id Slack API action's trigger identifier
/// @param team_id Slack Team ID which a user belongs to
/// @param user_id Slack User ID
/// @param bamboo_hr_org A BambooHR organization name which is used as the part of API URL
/// @param bamboo_hr_secret A BambooHR API secret
/// @param request_token Whether it should request additional permissions or just end up with error message
/// @returns TRUE on success or FALSE otherwise
static bool ProcessInstallCommand(
    const std::string& response_url,
    const std::string& trigger_id,
    const std::string& team_id,
    const std::string& user_id,
    const std::string& bamboo_hr_org,
    const std::string& bamboo_hr_secret,
    const bool request_token
) {
  using std::chrono::system_clock;

  LOG(DEBUG) << "Processing install command...";

  auto ReqToken = [=](const std::string& reason) {
    LOG(DEBUG) << "Starting request token workflow: " << reason;

    if (request_token) {
      // Store secret to database to proceed right after redirect
      json::value d;
      // Team ID and User ID should come with redirect request
      d[U("response_url")] = json::value::string(response_url);
      d[U("admin_user")] = json::value::string(user_id);
      d[U("bamboohr_org")] = json::value::string(bamboo_hr_org);
      d[U("bamboohr_secret")] = json::value::string(bamboo_hr_secret);
      d[U("time")] = json::value::number(system_clock::to_time_t(system_clock::now()));

      if (!DB::GetInstance().PutInstallCallback(trigger_id, d)) {
        LOG(ERROR) << "Could not store install callback to database:";
        PostToResponseURL(response_url, "Sorry, internal error occurred. Please try again later.");

        return false;
      }

      LOG(DEBUG) << "Responding to install command with request of additional permissions...";

      // Respond with install button which triggers request scope passing trigger_id as a code to check
      PostToResponseURL(
          response_url,
          R"JSN1({
    "text": "To allow application to change user's statuses it needs to request some additional permissions.",
    "attachments": [
        {
            "attachment_type": "default",
            "fallback": "Adding this command requires an official Slack client.",
            "actions": [
                {
                    "text": "Review Permissions",
                    "type": "button",
                    "url": "https://slack.com/oauth/authorize?client_id=)JSN1" + uri::encode_data_string(app_config.kSlackClientID)
              + "&state=" + uri::encode_data_string(trigger_id)
              + R"JSN2(&scope=users.profile:write,users.profile:read,users:read.email,users:read,commands"
                }
            ]
        }
    ]
})JSN2");
    } else {
      PostToResponseURL(
          response_url,
          "Sorry, I couldn't process your request because of unexpected error. " + reason
      );
    }

    return false;
  };

  LOG(DEBUG) << "Install: Checking if user_token exists in DB...";
  // Verify if user's token does already exist in the database
  json::value user_token;
  if (!DB::GetInstance().GetUserToken(team_id, user_id, &user_token)) {
    // There is no token for this user
    return ReqToken("Token does not exist.");
  }

  // Using token to check whether this user is privileged user
  json::value user_info;
  SlackApiClient api_client {user_token[U("access_token")].as_string()};

  LOG(DEBUG) << "Install: Checking Slack token scopes by requesting api.test endpoint...";

  // Checks required scopes
  // Retrieves all scopes which are associated with the token
  std::vector<std::string> scopes = api_client.GetScopes();
  std::sort(scopes.begin(), scopes.end());

  LOG(DEBUG) << "Install: Available scopes: " << boost::join(scopes, ", ");

  // Required OAuth scopes: Important! this vector should define sorted values
  const std::vector<std::string> required_scopes = {
      U("commands"),
      U("users.profile:read"),
      U("users.profile:write"),
      U("users:read"),
      U("users:read.email")
  };
  //std::sort(required_scopes.begin(), required_scopes.end());

  std::vector<std::string> diff_scopes;
  // Copies the elements from the sorted range [first1, last1) which are not found in the sorted range
  // [first2, last2) to the range beginning at d_first.
  std::set_difference(
      required_scopes.begin(), required_scopes.end(), scopes.begin(), scopes.end(),
      std::inserter(diff_scopes, diff_scopes.begin())
  );

  if (diff_scopes.size() > 0) {
    // One or more scopes are not qualified
    return ReqToken("Following scopes are missing: " + boost::join(diff_scopes, ", "));
  }

  LOG(DEBUG) << "Install: Trying to request user.info to check if the user is privileged...";

  try {
    user_info = api_client.UsersInfo(user_id);
  } catch (SlackApiError& e) {
    // Unable to request a user's info with specified token.
    if (!e.IsInvalidTokenError()) {
      // Some unexpected error is here
      LOG(ERROR) << "Unable to validate a token: " << e.what();
    }

    return ReqToken("Slack API request caused error: " + std::string(e.what()));
  }

  LOG(DEBUG) << "Install: User info details: " << user_info.serialize();

  if (false == user_info[U("user")][U("is_admin")].as_bool()
      && false == user_info[U("user")][U("is_owner")].as_bool()
      && false == user_info[U("user")][U("is_primary_owner")].as_bool()
  ) {
    // A user is not privileged user
    PostToResponseURL(response_url, "Sorry you're not workspace admin in Slack.");

    return false;
  }

  LOG(DEBUG) << "Install: Trying to fetch users list from BambooHR (" << bamboo_hr_org << ") validating API secret...";

  // Validating bambooHR credentials
  BambooHrApiClient bamboo_hr_api_client {bamboo_hr_secret, bamboo_hr_org};
  BambooHrUsersList bhr_list;
  SlackUsersList user_list;

  try {
    bhr_list = bamboo_hr_api_client.UsersList();
  } catch (BambooHrApiError& e) {
    PostToResponseURL(
        response_url,
        "Could not retrieve a list of users from BambooHR API with the specified organization name and token: "
            + std::string(e.what())
    );

    return false;
  }

  LOG(DEBUG) << "Install: Trying to fetch users list from Slack...";

  try {
    // Trying to get a list of the users in Slack
    user_list = api_client.UsersList(bhr_list);
  } catch (SlackApiError& e) {
    // Unable to request a users list with specified token.
    if (!e.IsInvalidTokenError()) {
      // Some unexpected error is here
      LOG(ERROR) << "Install: Could not retrieve Slack users list: " << e.what();
    }

    PostToResponseURL(
        response_url,
        "Could not retrieve Slack users list: " + std::string(e.what())
    );

    return false;
  }

  if (user_list.size() == 0) {
    PostToResponseURL(
        response_url,
        "Could not find anyone from BambooHR in Slack. "
        "Users should have the same emails in both applications."
    );

    return false;
  }

  LOG(DEBUG) << "Install: Finishing installation by storing TEAM record to database...";

  if (!DB::GetInstance().PutOrg(bamboo_hr_org, bamboo_hr_secret, team_id, user_id)) {
    PostToResponseURL(
        response_url,
        "Could not save API token due to internal error. Please try later."
    );

    return false;
  }

  LOG(DEBUG) << "Install: Responding with a successful message to Slack...";

  PostToResponseURL(response_url, "Congratulations! Now your team profile statuses will be synchronizing with BambooHR.");

  return true;
}

/// @brief Handle Slack redirect request
/// @param message A HTTP request message
static void HandleSlackRedirect(http_request& message) {
  // OAuth2 redirect when user allows application permissions
  auto params = uri::split_query(message.request_uri().query());
  auto code = params.count(U("code")) ? url_decode(params[U("code")]) : "";
  auto state = params.count(U("state")) ? url_decode(params[U("state")]) : "";
  auto error = params.count(U("error")) ? url_decode(params[U("error")]) : "";

  // We must sanitize key as it's used as a part of the key of a database record
  boost::trim_if(state, boost::is_cntrl() || boost::is_space());
  boost::trim_if(code, boost::is_cntrl() || boost::is_space());

  LOG(DEBUG) << "Redirect request: " << message.request_uri().query();

  if (error != "") {
    if (state != "") {
      // Trying to delete previously saved callback if it exists.
      DB::GetInstance().DeleteInstallCallback(state);
    }
    // User has chosen not to proceed with the authorization.
    // Redirecting to the main page.
    http_response response(status_codes::TemporaryRedirect);
    response.headers().add(U("Location"), U("/?error=user_denial"));
    message.reply(response);

    return;
  } else if (code == "") {
    message.reply(status_codes::BadRequest);

    return;
  }

  LOG(DEBUG) << "Redirect request: Requesting OAuth token...";

  // Requesting Slack user token
  bs::SlackApiClient client("");
  json::value value = client.OauthAccess(app_config.kSlackClientID, app_config.kSlackClientSecret, code);

  if (!value.at(U("ok")).is_null()
      && value[U("ok")].as_bool() == true
      && !value.at(U("access_token")).is_null()
  ) {
    //Token received successfully. Storing to database.
    if (!DB::GetInstance().PutUserToken(
        value[U("team_id")].as_string(),
        value[U("user_id")].as_string(),
        value
    )) {
      http_response response(status_codes::TemporaryRedirect);
      response.headers().add(U("Location"), U("/?error=db_error"));
      message.reply(response);

      return;
    }

    LOG(INFO) << "User: " << value[U("user_id")].as_string()
              << " Team: " << value[U("team_id")].as_string()
              << " installed application";
  } else {
    // Error oauth.access response
    http_response response(status_codes::TemporaryRedirect);
    response.headers().add(U("Location"), U("/?error=oauth_error"));
    message.reply(response);

    return;
  }

  // Redirects to a main page in case of a success
  http_response response(status_codes::TemporaryRedirect);
  response.headers().add(U("Location"), U("/?success=1"));
  message.reply(response);

  // Checking if there is Install callback as a part of request permissions workflow
  if (state != "") {
    LOG(DEBUG) << "Redirect request: Trying to finish install request...";
    // Trying to get callback data
    json::value cb_data;
    if (DB::GetInstance().GetInstallCallback(state, &cb_data)) {
      LOG(DEBUG) << "Redirect request: Callback found. Processing Install command again...";

      try {
        // It does exist so we have to finish install workflow by invoking action again
        ProcessInstallCommand(
            cb_data[U("response_url")].as_string(),
            state,
            value[U("team_id")].as_string(),
            value[U("user_id")].as_string(),
            cb_data[U("bamboohr_org")].as_string(),
            cb_data[U("bamboohr_secret")].as_string(),
            false
        );
      } catch (std::exception& e) {
        LOG(ERROR) << "Unable to reprocess install command: " << e.what();
      }

      LOG(DEBUG) << "Redirect request: Removing callback from database...";
      // Remove callback as it's only considered to be two step workflow
      DB::GetInstance().DeleteInstallCallback(state);
    }
  }
}

/// @brief Slash command handler
/// @param message A HTTP request message
static void HandleSlashCommandRequest(http_request& message) {
  using std::chrono::system_clock;

  // Gets x-www-form-urlencoded payload from response body
  auto payload = message.extract_string().get();

  auto sig_iter {message.headers().find("X-Slack-Signature")};
  auto ts_iter {message.headers().find("X-Slack-Request-Timestamp")};
  if (sig_iter == message.headers().end() || ts_iter == message.headers().end()) {
    message.reply(status_codes::BadRequest, "Some header is missing.");

    return;
  }

  // Gets current timestamp
  std::time_t tt = system_clock::to_time_t(system_clock::now());
  long int t = 0;
  try {
    t = std::stoi(ts_iter->second);
  } catch (...) {
    message.reply(status_codes::BadRequest, "Invalid timestamp.");

    return;
  }
  // Checks if the timestamp is not stale
  if (std::abs(tt - t) > 1000) {
    message.reply(
        status_codes::Forbidden,
        "Clock skew: " + std::to_string(std::abs(tt - t)) + "."
    );

    return;
  }
  // Validates signature
  auto hash {"v0=" + hmac_sha256("v0:" + ts_iter->second + ":" + payload, app_config.kSlackSigningSecret)};
  if (hash != sig_iter->second) {
    message.reply(status_codes::Forbidden, "Signature does not match.");

    return;
  }

  // Retrieves payload options
  auto params = uri::split_query(payload);

  LOG(DEBUG) << "Payload: " << payload;

  // Parsing command options
  const std::string team_id {!params.at("team_id").empty() ? url_decode(params["team_id"]) : ""};
  const std::string user_id {!params.at("user_id").empty() ? url_decode(params["user_id"]) : ""};
  const std::string trigger_id {!params.at("trigger_id").empty() ? url_decode(params["trigger_id"]) : ""};
  const std::string response_url {!params.at("response_url").empty() ? url_decode(params["response_url"]) : ""};

  if (!std::regex_match(team_id, kRegexAlphanum) || !std::regex_match(user_id, kRegexAlphanum)) {
    message.reply(
        status_codes::BadRequest,
        "Invalid payload: team_id and user_id must be nonempty alphanumeric strings."
    );

    return;
  }
  // Validating trigger identifier
  if (!std::regex_match(trigger_id, std::regex("[a-z0-9_\\.]+", std::regex::icase))) {
    message.reply(
        status_codes::BadRequest,
        "Invalid payload: trigger_id must be nonempty alphanumeric strings."
    );

    return;
  }

  if (!std::regex_match(response_url, std::regex("^https://.+"))) {
    message.reply(
        status_codes::BadRequest,
        "Invalid response URL."
    );

    return;
  }

  // At this point request is considered to be valid

  std::string input {params.at("text").empty() ? "" : url_decode(params["text"])};
  // Removing trailing spaces
  boost::trim_if(input, boost::is_cntrl() || boost::is_space());
  std::vector<std::string> tokens;

  if (input != "") {
    boost::split(tokens, input, boost::is_any_of(" \t"));

    if (tokens[0] == "install") {
      LOG(DEBUG) << "Starting install command...";
      // install command
      if (tokens.size() != 3
          || !std::regex_match(tokens[1], kRegexWord)
          || !std::regex_match(tokens[2], std::regex("[a-f0-9]{40,}", std::regex::icase))
      ) {
        message.reply(status_codes::OK, kCommandUsage);

        return;
      }

      // All further responses should go through response URL
      message.reply(status_codes::OK);

      try {
        ProcessInstallCommand(
            response_url,
            trigger_id,
            team_id,
            user_id,
            tokens[1],
            tokens[2],
            true
        );
      } catch (std::exception& e) {
        LOG(ERROR) << "Unable to process install command: " << e.what();
        PostToResponseURL(response_url, "Sorry, internal error occurred. Please try again later.");

        return;
      }

      return;
    } else {
      // Invalid command
      message.reply(status_codes::OK, kCommandUsage);

      return;
    }
  }

  LOG(DEBUG) << "Responding to " << kCommandName << " command...";

  // Gets who is out data for the requested Slack Team ID
  std::string wio_data = DB::GetInstance().GetWioData(team_id);

  // Builds a response with text message to say who is out today
  json::value response;
  response["text"] = json::value::string(wio_data != "" ? wio_data : "Nothing found.");

  message.reply(status_codes::OK, response);
}

void AppController::HandleGet(http_request message) {
  auto path = RequestPath(message);
  if (path.empty()) {
    // This is the main page of the application service
    concurrency::streams::istream str(OPEN_R<utf8char>("index.html").get());
    message.reply(status_codes::OK, str, kContentTypeTextHTMLCharsetUTF8);
  } else if (path[0] == "redirect") {
    HandleSlackRedirect(message);
  } else if (path[0] == "interactive" || path[0] == "command") {
    http_response response(status_codes::MethodNotAllowed);
    response.headers().add(U("Allow"), U("POST"));
    message.reply(response);
  } else {
    message.reply(status_codes::NotFound);
  }
}

void AppController::HandlePost(http_request message) {
  auto path = RequestPath(message);
  if (path.empty()) {
    http_response response(status_codes::MethodNotAllowed);
    response.headers().add(U("Allow"), U("GET"));
    message.reply(response);
  } else if (path[0] == "interactive") {
    LOG(DEBUG) << "Interactive: " << message.to_string() << message.extract_string().get();
    message.reply(status_codes::OK);
  } else if (path[0] == "command") {
    HandleSlashCommandRequest(message);
  } else {
    message.reply(status_codes::NotFound);
  }
}
} // namespace bs