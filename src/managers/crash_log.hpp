#pragma once

namespace showcase {

class CrashLogManager {
 public:
  static CrashLogManager& get();

  void checkLatest();

 private:
  CrashLogManager() = default;
  CrashLogManager(CrashLogManager const&) = delete;
  CrashLogManager& operator=(CrashLogManager const&) = delete;
};

}
