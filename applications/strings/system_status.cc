#include "system_status.h"

#include <cinttypes>

#include "pw_string/string_builder.h"

namespace system_status {

const char* GetStatusString(unsigned led_state) {
  pw::StringBuffer<64> sb;
  sb << "[SystemStatus] ";
  sb.Format("LED: %02u", led_state);
  return sb.data();
}

}  // namespace system_status
