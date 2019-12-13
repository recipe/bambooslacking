#include <sys/file.h>
#include <iomanip>
#include <thread>
#include <functional>
#include <chrono>
#include "app.h"
#include "app_controller.h"
#include "db.h"

#define BOOST_SPIRIT_THREADSAFE

INITIALIZE_EASYLOGGINGPP

/// @brief starts periodic tack in the separate thread
/// @param func lambda function to invoke
/// @param interval_sec interval in seconds to start
void timer_start(std::function<void(void)> func, unsigned int interval_sec) {
  std::thread([func, interval_sec]() {
    while (true) {
      auto x = std::chrono::steady_clock::now() + std::chrono::seconds(interval_sec);

      try {
        func();
      } catch (web::http::http_exception& e) {
        LOG(ERROR) << "Sync: HTTP Error: " << e.what();
      } catch (std::runtime_error& e) {
        LOG(ERROR) << "Sync: Runtime Error: " << e.what();
      } catch (std::exception& e) {
        LOG(ERROR) << "Sync: Fatal Error: " << e.what();
      }

      std::this_thread::sleep_until(x);
    }
  }).detach();
}

static int DropPrivileges(uid_t new_uid) {
  if (setreuid(new_uid, new_uid) < 0) {
    return -1;
  }

  if (getuid() != new_uid
      || geteuid() != new_uid
  ) {
    return -1;
  }

  return 0;
}

int main(int argc, char* argv[]) {
  // Our process ID and Session ID
  pid_t pid, sid;

  // Loading config
  if(!bs::LoadConfig()) {
    exit(EXIT_FAILURE);
  }

  // it's platform specific file operations
  int pid_file = open(bs::kPidFile, O_CREAT | O_RDWR, 0666);
  int rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if (rc) {
    if (EWOULDBLOCK == errno) {
      // another instance is running
      std::cout << "Another instance is running. PID_FILE " << bs::kPidFile << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  // Fork off the parent process
  pid = fork();
  if (pid < 0) {
    std::cout << "Could not fork the parent process" << std::endl;
    exit(EXIT_FAILURE);
  }

  // If we got a good PID, then we can exit the parent process.
  if (pid > 0) {
    exit(EXIT_SUCCESS);
  }

  // Change the file mode mask
  umask(0);

  // Configuring a logger
  el::Configurations defaultConf;
  defaultConf.setToDefault();
  // To set GLOBAL configurations you may use
  defaultConf.setGlobally(el::ConfigurationType::Format, "%datetime - %level - %msg");
  defaultConf.setGlobally(el::ConfigurationType::ToFile, "true");
  defaultConf.setGlobally(el::ConfigurationType::Filename, bs::kLogFile);
  defaultConf.set(el::Level::Debug, el::ConfigurationType::Enabled, "true");
  defaultConf.set(el::Level::Info, el::ConfigurationType::Enabled, "true");
  defaultConf.set(el::Level::Error, el::ConfigurationType::Enabled, "true");
  defaultConf.set(el::Level::Fatal, el::ConfigurationType::Enabled, "true");
  el::Loggers::reconfigureLogger("default", defaultConf);

  // Create a new SID for the child process
  sid = setsid();
  if (sid < 0) {
    LOG(FATAL) << "Could not create a new SID for the child process";
    exit(EXIT_FAILURE);
  }

  // writing process identifier to PID_FILE
  const std::string s = std::to_string(sid);
  if (-1 == write(pid_file, s.c_str(), s.size())) {
    LOG(FATAL) << "Could not write process identifier to PID_FILE";
    exit(EXIT_FAILURE);
  }

  // Change the current working directory
  if ((chdir("/")) < 0) {
    LOG(FATAL) << "Could not change the current working directory to /";
    exit(EXIT_FAILURE);
  }

  // Close out the standard file descriptors
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  // Daemon-specific initialization goes here
  LOG(INFO) << "Starting service...";
  bs::InterruptHandler::HookSIGINT();

  bs::AppController server;

  try {
    server.SetEndpoint(bs::app_config.kServerEndpoint);
  } catch (std::exception& e) {
    LOG(FATAL) << "Unable to set server endpoint. Error: " << e.what();
    exit(EXIT_FAILURE);
  }

  try {
    // Wait for server initialization
    server.Accept().wait();
    LOG(INFO) << "Starting listening for requests at: " << server.Endpoint();
  } catch (std::exception& e) {
    LOG(FATAL) << "Unable to start server. Error: " << e.what();
    exit(EXIT_FAILURE);
  }

  // Start periodic tack to sync user profile statuses in separate thread
  timer_start(bs::SyncUserProfileStatuses, 600);

  try {
    bs::InterruptHandler::WaitForUserInterrupt();
    server.Shutdown().wait();
  } catch (std::exception& e) {
    LOG(FATAL) << "Error while shutdown: " << e.what();
    exit(EXIT_FAILURE);
  } catch (...) {
    LOG(FATAL) << "Fatal Error!";
    bs::RuntimeUtils::PrintStackTrace();
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}