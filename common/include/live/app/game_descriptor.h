#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace live {

struct GameDescriptor {
  std::string app_name;
  std::string display_name;
  std::optional<std::filesystem::path> development_game_root;
};

}
