#include <leveldb/db.h>
#include "encryption.h"
#include "db.h"

using namespace web;

namespace bs {
bool DB::GetOrgs(std::map<std::string, json::value>* res) {
  leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
  for (it->Seek(kTeamPrefix + ":"); it->Valid() && it->key().ToString() < kTeamPrefix + "~"; it->Next()) {
    leveldb::Slice p = it->key();
    p.remove_prefix(kTeamPrefix.size() + 1);
    std::string slack_team_id = p.ToString();
    auto value = json::value::parse(decrypt(it->value().ToString(), kCryptokey));

    if (!value.is_object()) {
      // invalid data
      continue;
    }

    json::value user_token;
    if (!GetUserToken(slack_team_id, value[U("admin_user")].as_string(), &user_token)) {
      continue;
    }

    value[U("token")] = user_token;

    (*res)[slack_team_id] = value;
  }

  return true;
}

bool DB::PutWioData(const std::string& slack_team_id, const std::string& message) {
  auto s = db->Put(
      leveldb::WriteOptions(),
      kWhoIsOutPrefix + ":" + slack_team_id,
      encrypt(message, kCryptokey)
  );
  return s.ok() ? true : false;
}

std::string DB::GetWioData(const std::string& slack_team_id) {
  std::string message;
  auto s = db->Get(
      leveldb::ReadOptions(),
      kWhoIsOutPrefix + ":" + slack_team_id,
      &message
  );
  return s.ok() ? decrypt(message, kCryptokey) : "";
}

bool DB::GetUserToken(const std::string& slack_team_id, const std::string& slack_user_id, json::value* res) {
  std::string token;
  // Get admin token for value
  leveldb::Status s = db->Get(
      leveldb::ReadOptions(),
      kUserPrefix + ":" + slack_user_id + ":" + slack_team_id,
      &token
  );
  if (!s.ok()) {
    // there is no token in database
    return false;
  }

  (*res) = json::value::parse(decrypt(token, kCryptokey));
  if (!(*res).is_object()) {
    // invalid data
    return false;
  }

  return true;
}

bool DB::PutUserToken(const std::string& slack_team_id, const std::string& slack_user_id, const std::string& token) {
  leveldb::Status s = db->Put(
      leveldb::WriteOptions(),
      kUserPrefix + ":" + slack_user_id + ":" + slack_team_id,
      encrypt(token, kCryptokey)
  );

  return s.ok() ? true : false;
}

bool DB::PutOrg(
    const std::string& bamboo_hr_org,
    const std::string& bamboo_hr_secret,
    const std::string& slack_team_id,
    const std::string& slack_user_id
) {
  using web::json::value;
  value jv;
  jv[U("bamboohr_secret")] = value::string(bamboo_hr_secret);
  jv[U("bamboohr_org")] = value::string(bamboo_hr_org);
  jv[U("admin_user")] = value::string(slack_user_id);

  leveldb::Status s = db->Put(
    leveldb::WriteOptions(),
    kTeamPrefix + ":" + slack_team_id,
    encrypt(jv.serialize(), kCryptokey)
  );

  return s.ok() ? true : false;
}

bool DB::PutInstallCallback(
    const std::string& trigger_id,
    const web::json::value& data
) {
  leveldb::Status s = db->Put(
      leveldb::WriteOptions(),
      kCallbackPrefix + ":" + trigger_id,
      encrypt(data.serialize(), kCryptokey)
  );

  return s.ok() ? true : false;
}

bool DB::GetInstallCallback(const std::string& trigger_id, web::json::value* res) {
  std::string data;
  // Get admin token for value
  leveldb::Status s = db->Get(
      leveldb::ReadOptions(),
      kCallbackPrefix + ":" + trigger_id,
      &data
  );
  if (!s.ok()) {
    // there is no data in database
    return false;
  }

  (*res) = json::value::parse(decrypt(data, kCryptokey));
  if (!(*res).is_object()) {
    // invalid data
    return false;
  }

  return true;
}

bool DB::DeleteInstallCallback(const std::string& trigger_id) {
  leveldb::Status s = db->Delete(
      leveldb::WriteOptions(),
      kCallbackPrefix + ":" + trigger_id
  );

  return s.ok() ? true : false;
}
} // namespace bs