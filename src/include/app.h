#pragma once

#include <string>
#include <condition_variable>
#include <mutex>
#include <execinfo.h>
#include <unistd.h>
#include <csignal>
#include "slackapi.h"
#include "bamboohrapi.h"
#include "easylogging++.h"
#include "network_utils.h"

namespace bs {

/// @brief Loads config. Returns TRUE on success
bool LoadConfig();

/// @brief Synchronizes BambooHR user's profile statuses with Slack user's status
void SyncUserProfileStatuses();

class RuntimeUtils {
 public:
  static void PrintStackTrace() {
    el::base::debug::StackTrace();
  }
};

static std::condition_variable _condition;
static std::mutex _mutex;

class InterruptHandler {
 public:
  static void HookSIGINT() {
    signal(SIGINT, HandleUserInterrupt);
  }

  static void HandleUserInterrupt(int signal){
    if (signal == SIGINT) {
      LOG(DEBUG) << "SIGINT trapped...";
      _condition.notify_one();
    }
  }

  static void WaitForUserInterrupt() {
    std::unique_lock<std::mutex> lock { _mutex };
    _condition.wait(lock);
    LOG(DEBUG) << "User has signalled to interrupt a program...";
    lock.unlock();
  }
};
} //namespace bs