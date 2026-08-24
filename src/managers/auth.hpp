#pragma once

#include <Geode/Geode.hpp>

#include <mutex>

namespace showcase {

struct AccountAuth {
  int accountID = 0;
  std::string token;  // argon token, ofc... NOT GD's.
};

class AuthManager {
 public:
  static AuthManager& get();

  void authenticate();
  bool hasAccount() const;
  std::optional<AccountAuth> account() const;

 private:
  AuthManager() = default;
  AuthManager(AuthManager const&) = delete;
  AuthManager& operator=(AuthManager const&) = delete;

  void clearAccount();
  void setAccount(AccountAuth account);

  // mutex is needed because account is read in Arc runtime (basically every Client::request())
  // `mutable` because it's used in const functions
  mutable std::mutex m_accountMutex;
  std::optional<AccountAuth> m_account;
  geode::async::TaskHolder<geode::Result<std::string>> m_authentication;
};

}
