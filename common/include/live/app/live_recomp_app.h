#pragma once

#include <filesystem>
#include <functional>
#include <optional>

#include <rex/rex_app.h>

#include "live/app/game_descriptor.h"
#include "live/app/game_paths.h"
#include "live/input/controller_menu_watcher.h"
#include "live/platform/gaming_runtime_session.h"

namespace live {

class SettingsDialog;
class SystemMenuDialog;

class LiveRecompApp : public rex::ReXApp {
 protected:
  LiveRecompApp(rex::ui::WindowedAppContext& context, GameDescriptor descriptor,
                rex::PPCImageInfo image_info);

  void OnConfigurePaths(rex::PathConfig& paths) override;
  void OnPostInitLogging() override;
  void OnPreSetup(rex::RuntimeConfig& config) override;
  void OnConfigureStyle(ImGuiStyle& imgui_style, rex::ui::Style& overlay_style) override;
  void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override;
  std::optional<rex::PathConfig> OnFinalizePaths(const rex::PathConfig& defaults,
                                                 std::function<void(rex::PathConfig)> resume) override;
  void OnPostLoadXexImage() override;
  void OnPostSetup() override;
  void OnShutdown() override;

  const GameDescriptor& descriptor() const { return descriptor_; }

 private:
  bool InstallFromEnvironment(const std::filesystem::path& game_root);
  void InstallContentPackages();
  void ToggleSystemMenu();
  void OpenSystemMenu();
  void OpenSettings();

  GameDescriptor descriptor_;
  rex::PPCImageInfo image_info_;
  GamePaths paths_;
  std::filesystem::path game_data_root_;
  GamingRuntimeSession gaming_runtime_;
  ControllerMenuWatcher menu_watcher_;
  SystemMenuDialog* system_menu_ = nullptr;
  SettingsDialog* settings_dialog_ = nullptr;
};

}
