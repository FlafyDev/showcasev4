#include "auth.hpp"

#include <argon/argon.hpp>

using namespace geode::prelude;

namespace showcase {

AuthManager& AuthManager::get() {
  static AuthManager instance;
  return instance;
}

void AuthManager::authenticate() {
  if (!argon::signedIn()) {
    clearAccount();
    return;
  }

  auto account = argon::getGameAccountData();
  if (!account.valid()) {
    clearAccount();
    return;
  }

  clearAccount();

  auto authentication = argon::startAuth({
    .account = account,
    .forceStrong = true,
  });

  m_authentication.spawn("Showcase account authentication", std::move(authentication),
    [this, account = std::move(account)](Result<std::string> result) {
      if (result.isErr()) {
        clearAccount();
        log::warn("Showcase account authentication failed: {}", result.unwrapErr());
        return;
      }
      setAccount({account.accountId, std::move(result).unwrap()});
      log::debug("Showcase authenticated account {}", account.accountId);
    });
}

bool AuthManager::hasAccount() const {
  std::lock_guard lock(m_accountMutex);
  return m_account.has_value();
}

std::optional<AccountAuth> AuthManager::account() const {
  std::lock_guard lock(m_accountMutex);
  return m_account;
}

void AuthManager::clearAccount() {
  std::lock_guard lock(m_accountMutex);
  m_account.reset();
}

void AuthManager::setAccount(AccountAuth account) {
  std::lock_guard lock(m_accountMutex);
  m_account = std::move(account);
}

}
