#include "crash_log.hpp"

#include "client.hpp"

#include <Geode/Geode.hpp>
#include <Geode/loader/Dirs.hpp>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>

using namespace geode::prelude;

namespace showcase {

namespace {
constexpr size_t MAX_CRASH_LOG_BYTES = 1024 * 1024;
constexpr char INSTALLED_MODS_HEADER[] = "== Installed Mods ==";

struct CrashLog {
  std::filesystem::path path;
  std::filesystem::file_time_type modified;
  uintmax_t size;
};

std::optional<CrashLog> latestCrashLog() {
  std::error_code error;
  auto directory = dirs::getCrashlogsDir();
  std::optional<CrashLog> latest;

  for (std::filesystem::directory_iterator iterator(directory, error), end;
    !error && iterator != end; iterator.increment(error)) {
    if (!iterator->is_regular_file(error) || error) continue;

    auto modified = iterator->last_write_time(error);
    if (error) continue;

    auto size = iterator->file_size(error);
    if (error) continue;

    if (!latest || modified > latest->modified) {
      latest = CrashLog{iterator->path(), modified, size};
    }
  }

  return latest;
}

bool referencesShowcase(std::string const& contents) {
  auto installedMods = contents.find(INSTALLED_MODS_HEADER);
  return contents.substr(0, installedMods).find(Mod::get()->getID()) != std::string::npos;
}
}

CrashLogManager& CrashLogManager::get() {
  static CrashLogManager instance;
  return instance;
}

void CrashLogManager::checkLatest() {
  if (!Mod::get()->getSettingValue<bool>("share-crash-logs")) return;

  auto latestLog = latestCrashLog();
  if (!latestLog) return;

  auto logIdentifier = latestLog->path.filename();
  if (Mod::get()->getSavedValue<std::string>("checked-crash-log") == logIdentifier) return;
  Mod::get()->setSavedValue("checked-crash-log", logIdentifier);

  if (latestLog->size == 0 || latestLog->size > MAX_CRASH_LOG_BYTES) {
    return;
  }

  std::ifstream input(latestLog->path, std::ios::binary);
  if (!input) return;

  std::string contents(std::istreambuf_iterator<char>(input), {});
  if (!referencesShowcase(contents)) return;

  async::spawn(Client::get().crashReport(Mod::get()->getVersion().toVString(), std::move(contents)),
    [](Result<> result) {
      if (result.isErr()) {
        log::warn("Crash report upload failed: {}", result.unwrapErr());
      }
    });
}

}
