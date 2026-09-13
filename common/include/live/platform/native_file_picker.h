#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace live {

class NativeFilePicker {
 public:
  explicit NativeFilePicker(void* owner_window) : owner_window_(owner_window) {}

  static bool IsAvailable();
  std::optional<std::filesystem::path> PickDiscImage(const std::string& title) const;

 private:
  void* owner_window_;
};

}
