#include "base64.h"
#include "bamboohrapi.h"

using namespace web;
using namespace web::http;

namespace bs {
BambooHrApiClient::BambooHrApiClient(const std::string& kApiToken, const std::string& kOrgName)
    : kApiToken(kApiToken), kOrgName(kOrgName) {}

json::value BambooHrApiClient::SendRequest(const method& mtd, const std::string& uri) {
  client::http_client client(kApiUrl);
  http_request req(mtd);

  req.headers().add(U("Accept"), U("application/json"));
  req.headers().add(U("User-Agent"), GetUserAgent());
  req.headers().add(U("Accept-Charset"), U("utf-8"));
  // BambooHR API uses basic authentication
  req.headers().add(U("Authorization"), U("Basic " + base64_encode(kApiToken + ":x")));
  req.set_request_uri(uri);

  http_response response = client.request(req).get();

  if (response.status_code() != 200) {
    throw BambooHrApiError(response.status_code());
  }

  return response.extract_json().get();
}

BambooHrUsersList BambooHrApiClient::UsersList() {
  BambooHrUsersList users;

  // Build request URI and start the request.
  uri_builder builder(U("/api/gateway.php/" + kOrgName + "/v1/meta/users/"));

  json::value json_response = SendRequest(methods::GET, builder.to_string());

  auto object = json_response.as_object();

  for (auto i = object.begin(); i != object.end(); i++) {
    // Ignoring fired employees
    if (U("enabled") != i->second.at(U("status")).as_string())
      continue;

    users[i->second.at(U("email")).as_string()] = i->second.at(U("employeeId")).as_integer();
  }

  return users;
}

BambooHrTimeOffList BambooHrApiClient::WhoIsOut(const std::string& start_date, const std::string& end_date) {
  BambooHrTimeOffList ret;

  if (start_date == "" || end_date == "") {
    throw std::invalid_argument("date should be nonempty and provided in the Y-m-d format.");
  }

  if (start_date > end_date) {
    throw std::logic_error("start_date cannot be more than end_date.");
  }

  // Build request URI and start the request.
  uri_builder builder(U("/api/gateway.php/" + kOrgName + "/v1/time_off/requests/"));
  builder.append_query(U("status"), U("approved"));
  builder.append_query(U("start"), start_date);
  builder.append_query(U("end"), end_date);

  json::value json_response = SendRequest(methods::GET, builder.to_string());

  auto array = json_response.as_array();

  for (int i = 0; i < array.size(); ++i) {
    auto employee_id = std::stoi(array[i].at(U("employeeId")).as_string());
    auto timeoff_type = array[i].at(U("type"))[U("name")].as_string();
    auto dates = array[i].at(U("dates")).as_object();
    // Pick only dates where time off is active in the specified period
    for(auto v = std::begin(dates); v != std::end(dates); v++) {
      if (start_date <= v->first && v->first <= end_date) {
        // Is there any value for current employee
        auto it {ret.find(employee_id)};
        if (it == ret.end()) {
          // Initialize the value for the employee
          ret[employee_id] = {{v->first, {{timeoff_type}}}};

          continue;
        }
        // Is there any value for the current date with different time-off type?
        auto it2 {it->second.find(v->first)};
        if (it2 == std::end(it->second)) {
          // Initialize the value for the current date
          it->second[v->first] = {{timeoff_type}};

          continue;
        }
        // There is some value for the current date already.
        // We should add another time off type.
        it2->second.push_back(timeoff_type);
      }
    }
  }

  return ret;
}
} // namespace bs