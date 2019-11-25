#include "common.h"

int main(int argc, char* argv[]) {
  std::cout << "Hello world!" << std::endl;
  // TEAM:{SLACK_TEAMID} => {bamboohr_secret:"", bamboohr_org:"", admin_user:"{SLACK_USERID}"}
  // USER:{SLACK_USER_ID}:{SLACK_TEAMID} => JSON_TOKEN
  // WIO:{SLACK_TEAMID} => "Formatted text"

  // setup workflow:
  // user adds application to slack -> store a token only if it's privileged user
  // user executes slack command /bs install {org} {secret}
      // check if user is slack privileged user, if not - error
      // check if user token exist or store it

  // if we need to change user's profile for admin user - look up for her token in db.

  // subscribe to Events when user grants or revokes some permission or token from the application
      // if token exists - do proper action

  // OPTIONALLY if workspace admin token became invalid - try to use token of another admin from this team
}