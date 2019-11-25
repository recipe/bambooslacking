#pragma once

#include <leveldb/db.h>
#include <cpprest/json.h>
#include "common.h"

namespace bs {
/// @brief DB operations
class DB {
 public:
  static DB& GetInstance() {
    static DB instance {app_config.kCryptokey};
    // Instantiated on first use.
    return instance;
  }
  DB(DB const&) = delete;
  void operator=(DB const&) = delete;

  inline static const std::string kTeamPrefix = "TEAM";
  inline static const std::string kUserPrefix = "USER";
  inline static const std::string kWhoIsOutPrefix = "WIO";
  inline static const std::string kCallbackPrefix = "CALLBACK";

  /// @brief Gets all organization to process who is out
  /// @param res A map where key is slack team ID and value is json object that contain organization metadata
  /// @returns TRUE on success or FALSE otherwise
  bool GetOrgs(std::map<std::string, web::json::value>* res);

  /// @brief Add new or replace existing organization
  /// @param bamboo_hr_org An organization name as it's used in the API url
  /// @param bamboo_hr_secret An BambooHR API secret
  /// @param slack_team_id Slack team identifier
  /// @param slack_user_id Slack admin user identifier
  bool PutOrg(
    const std::string& bamboo_hr_org,
    const std::string& bamboo_hr_secret,
    const std::string& slack_team_id,
    const std::string& slack_user_id
  );

  /// @brief Gets user's token
  /// @param slack_team_id Slack team ID
  /// @param slack_user_id Slack user ID
  /// @param res A token
  /// @returns TRUE on success or FALSE otherwise
  bool GetUserToken(const std::string& slack_team_id, const std::string& slack_user_id, web::json::value* res);

  /// @brief Puts user's token
  /// @param slack_team_id Slack team ID
  /// @param slack_user_id Slack user ID
  /// @param token JSON string
  /// @returns Returns TRUE on success or FALSE on failure
  bool PutUserToken(const std::string& slack_team_id, const std::string& slack_user_id, const std::string& token);

  /// @brief Puts user's token
  /// @param slack_team_id Slack team ID
  /// @param slack_user_id Slack user ID
  /// @param token A token
  /// @returns Returns TRUE on success or FALSE on failure
  bool PutUserToken(
      const std::string& slack_team_id,
      const std::string& slack_user_id,
      const web::json::value& token
  ) {
    return PutUserToken(slack_team_id, slack_user_id, token.serialize());
  }

  /// @brief Puts install callback to handle in future
  /// @param trigger_id A Slack API action's trigger identifier
  /// @param data object with all necessary parameters to complete install action
  /// @returns TRUE on success or FALSE otherwise
  bool PutInstallCallback(
    const std::string& trigger_id,
    const web::json::value& data
  );

  /// @brief Gets install callback to proceed
  /// @param trigger_id A Slack API action's trigger identifier
  /// @param res An output data object to set
  /// @returns TRUE on success or FALSE otherwise
  bool GetInstallCallback(const std::string& trigger_id, web::json::value* res);

  /// @brief Deletes install callback from database
  /// @param trigger_id A Slack API action's trigger identifier
  /// @returns TRUE on success or FALSE otherwise
  bool DeleteInstallCallback(const std::string& trigger_id);

  /// @brief Saves who-is-out message to database
  /// @param slack_team_id A Slack team ID
  /// @param message A message
  /// @returns Returns TRUE on success or FALSE on failure
  bool PutWioData(const std::string& slack_team_id, const std::string& message);

  /// @brief Gets who-is-out message from database
  /// @param slack_team_id A Slack team ID
  /// @returns Returns a message on success or empty string otherwise
  std::string GetWioData(const std::string& slack_team_id);

 protected:
  /// @brief leveldb database instance
  leveldb::DB* db;
  /// @brief leveldb database options
  leveldb::Options options;
  /// @brief cryptokey
  std::string kCryptokey;
 private:
  DB(const std::string& cryptokey) {
    kCryptokey = cryptokey;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, kDbName, &db);
    if (!status.ok()) {
      throw std::runtime_error("Could not connect to database " + kDbName);
    }
  }

  ~DB() {
    delete db;
  }
};
} // namespace bs