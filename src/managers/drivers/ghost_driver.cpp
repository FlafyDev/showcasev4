#include "managers/drivers/ghost_driver.hpp"
#include "managers/session.hpp"

namespace showcase {

GhostDriver::GhostDriver(ReplaySession* session, bool enabled)
  : m_session(session), m_enabled(enabled) {}

}
