#pragma once

#include <rex/image_info.h>

namespace rex {
class Runtime;
}

namespace live {

class GuestImageDump {
 public:
  static constexpr const char* kEnvironmentVariable = "LIVE_RECOMP_DUMP_IMAGE";

  static void WriteAndExitIfRequested(rex::Runtime& runtime, const rex::PPCImageInfo& image);
};

}
