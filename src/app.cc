#include "common.h"
#include "app.h"
#include "db.h"

namespace bs {

bool LoadConfig() {
  std::ifstream ifs(kConfigFile, std::ifstream::in);
  if (!ifs.is_open()) {
    std::cout << "Error: Unable to open config file " << kConfigFile << " for reading."
              << std::endl;
    return false;
  }

  auto v = web::json::value::parse(ifs);
  ifs.close();

  app_config.kSlackClientID =
      !v.at(kCfgSlackClientID).is_null()
      ? v.at(kCfgSlackClientID).as_string()
      : "";

  if (app_config.kSlackClientID == "") {
    std::cout << "Error: " << kCfgSlackClientID << " has not been set in " << kConfigFile << "."
              << std::endl;
    return false;
  }

  app_config.kSlackClientSecret =
      !v.at(kCfgSlackClientSecret).is_null()
      ? v.at(kCfgSlackClientSecret).as_string()
      : "";

  if (app_config.kSlackClientSecret == "") {
    std::cout << "Error: " << kCfgSlackClientSecret << " has not been set in " << kConfigFile << "."
              << std::endl;
    return false;
  }

  app_config.kSlackSigningSecret =
      !v.at(kCfgSlackSigningSecret).is_null()
      ? v.at(kCfgSlackSigningSecret).as_string()
      : "";

  if (app_config.kSlackSigningSecret == "") {
    std::cout << "Error: " << kCfgSlackSigningSecret << " has not been set in " << kConfigFile << "."
              << std::endl;
    return false;
  }

  app_config.kCryptokey =
      !v.at(kCfgCryptokey).is_null()
      ? v.at(kCfgCryptokey).as_string()
      : "";

  if (app_config.kCryptokey == "") {
    std::cout << "Error: " << kCfgCryptokey << " has not been set in " << kConfigFile << "."
              << std::endl;
    return false;
  }

  app_config.kSSLKeyFile =
      !v.at(kCfgSSLKey).is_null()
      ? v.at(kCfgSSLKey).as_string()
      : "";

  app_config.kSSLCertFile =
      !v.at(kCfgSSLCert).is_null()
      ? v.at(kCfgSSLCert).as_string()
      : "";

  app_config.kSSLChainFile =
      !v.at(kCfgSSLFullchain).is_null()
      ? v.at(kCfgSSLFullchain).as_string()
      : "";

  app_config.kSSLContextPassword =
      !v.at(kCfgSSLContextPassword).is_null()
      ? v.at(kCfgSSLContextPassword).as_string()
      : "";

  app_config.kServerEndpoint =
      !v.at(kCfgServerEndpoint).is_null()
      ? v.at(kCfgServerEndpoint).as_string()
      : "";

  if (app_config.kServerEndpoint == "") {
    std::cout << "Error: " << kCfgServerEndpoint << " has not been set in " << kConfigFile << "."
              << std::endl;
    return false;
  }

  return true;
}

void SyncUserProfileStatuses() {
  std::map<std::string, web::json::value> teams;

  if (!DB::GetInstance().GetOrgs(&teams)) {
    LOG(ERROR) << "Could not retrieve a list of organizations from database";
  }

  // Retrieves who is out data for all organizations one by one
  for (const auto& [slack_team_id, org_val] : teams) {
    std::string slack_admin_user_id = org_val.at(U("admin_user")).as_string();
    std::string bhr_org = org_val.at(U("bamboohr_org")).as_string();
    std::string bhr_secret = org_val.at(U("bamboohr_secret")).as_string();
    std::string slack_token = org_val.at(U("token")).at(U("access_token")).as_string();

    SlackApiClient slack_api_client(slack_token);
    BambooHrApiClient bamboohr_api_client(bhr_secret, bhr_org);
    // Prefetch all bambooHR active employees
    BambooHrUsersList bamboohr_users = bamboohr_api_client.UsersList();
    // Who is out data for the team
    std::vector<std::string> wio_data;
    // Gets all users from Slack who exist in the bambooHR
    SlackUsersList slack_users = slack_api_client.UsersList(bamboohr_users);

    // Get time offs schedule between yesterday and tomorrow
    BambooHrTimeOffList timeoff_list = bamboohr_api_client.WhoIsOut(
        GetCurrentTimestamp("%Y-%m-%d", -90000),
        GetCurrentTimestamp("%Y-%m-%d", 90000)
    );

    LOG(INFO) << "Who is out today?";

    for (const auto& [user_email, user] : slack_users) {
      // Is there time-off for the current employee?
      auto iter = timeoff_list.find(user.bamboohr_employee_id);
      if (iter == timeoff_list.end()) {
        // time off does not exist
        continue;
      }

      // Get user's date in her timezone. If user is working in america her date can be different from the Europe.
      auto user_cur_date = GetCurrentTimestamp("%Y-%m-%d", user.tz_offset);

      auto it = iter->second.find(user_cur_date);
      if (it == std::end(iter->second)) {
        // time off for the current date does not exist
        continue;
      }

      // If user has more than one time off type for the selected day it chooses an appropriate one based on priorities.
      // Then less int value of type then more priority.
      if (it->second.size() > 1) {
        sort(it->second.begin(), it->second.end());
      }

      // The name of the accepted time-off type
      std::string time_off_type = it->second.front();
      // user's time off profile to apply
      TimeOffProfile time_off_profile_to_apply;

      auto time_off_iter = TimeOff::NAMES.find(time_off_type);
      if (time_off_iter == TimeOff::NAMES.end()) {
        // Unknown type
        time_off_profile_to_apply = {
            time_off_type,
            U(":grey_question:"),
            time_off_type
        };
      } else {
        time_off_profile_to_apply = TimeOff::TYPES.at(time_off_iter->second);
      }

      // offset should be subtracted from the timestamp in UTC TZ as to be just in time in the user's TZ
      auto expected_status_expiration = strtotime(user_cur_date + " 23:59:59") - user.tz_offset;

      // Creating "who is out" record for this user
      std::stringstream line;
      line << "<@" << user.slack_id << "> (" << user.real_name << ") "
           << time_off_profile_to_apply.text << " " << time_off_profile_to_apply.emoji;

      wio_data.push_back(line.str());

      // Should we change anything?
      if (user.status_emoji == time_off_profile_to_apply.emoji &&
          user.status_expiration == expected_status_expiration) {
        LOG(INFO) << "Skipping. " << user.real_name << " has actual status"
                  << " - slackId: " << user.slack_id
                  << " - employeeId: " << user.bamboohr_employee_id
                  << " - tz_off: " << user.tz_offset
                  << " - emoji: " << time_off_profile_to_apply.emoji
                  << " - exp: " << "'" << user_cur_date << "' " << expected_status_expiration
                  << " - old (status: " << user.status_text
                  << ", emoji: " << user.status_emoji
                  << ", exp: " << user.status_expiration << ")";
        continue;
      }

      LOG(INFO) << "Setting '" << time_off_profile_to_apply.text << "' status for " << user.real_name
                << " - slackId: " << user.slack_id
                << " - employeeId: " << user.bamboohr_employee_id
                << " - tz_off: " << user.tz_offset
                << " - emoji: " << time_off_profile_to_apply.emoji
                << " - exp: " << "'" << user_cur_date << "' " << expected_status_expiration
                << " - old (status: " << user.status_text
                << ", emoji: " << user.status_emoji
                << ", exp: " << user.status_expiration << ")";

      if (user.is_privileged && user.slack_id != slack_admin_user_id) {
        //If user is admin we should try to use his own token if it exists
        web::json::value atoken;
        if (DB::GetInstance().GetUserToken(slack_team_id, user.slack_id, &atoken)) {
          // Token has been found. Updating user's profile status
          SlackApiClient aclient(atoken[U("access_token")].as_string());
          aclient.UsersProfileSetStatus(
              user.slack_id,
              time_off_profile_to_apply,
              expected_status_expiration
          );
        }
      } else {
        // Sets user's status in Slack
        slack_api_client.UsersProfileSetStatus(
            user.slack_id,
            time_off_profile_to_apply,
            expected_status_expiration
        );
      }
    }

    if (wio_data.empty()) {
      LOG(INFO) << "No time offs found.";
      wio_data.push_back("Everybody is on board.");
    }

    //Put who is out data to database
    if (!DB::GetInstance().PutWioData(slack_team_id, boost::algorithm::join(wio_data, "\n"))) {
      LOG(ERROR) << "Unable to store who-is-out information to database.";
    }
  }
}
} //namespace bs