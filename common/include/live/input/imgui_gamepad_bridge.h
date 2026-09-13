#pragma once

struct ImGuiIO;

namespace live {

class ImGuiGamepadBridge {
 public:
  static void FeedPrimaryController(ImGuiIO& io);
};

}
