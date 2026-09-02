#include "managers/drivers/input_visualizer_driver.hpp"
#include "managers/session.hpp"

namespace showcase {

InputVisualizerDriver::InputVisualizerDriver(ReplaySession* session, bool enabled)
  : m_session(session), m_enabled(enabled) {}

}
