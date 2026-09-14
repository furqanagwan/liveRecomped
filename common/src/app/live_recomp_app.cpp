#include "live/app/live_recomp_app.h"

#include <cstdlib>

#include <rex/cvar.h>
#include <rex/input/device_assignment.h>
#include <rex/input/input_system.h>
#include <rex/logging.h>
#include <rex/ui/keybinds.h>
#include <rex/ui/window.h>
#include <rex/ui/windowed_app_context.h>

#include "live/debug/guest_image_dump.h"
#include "live/installer/disc_image_installer.h"
#include "live/settings/user_settings_store.h"
#include "live/ui/disc_install_dialog.h"
#include "live/ui/monochrome_theme.h"
#include "live/ui/settings_dialog.h"
#include "live/ui/system_menu_dialog.h"

REXCVAR_DECLARE(bool, live_shared_controllers);

namespace live {

namespace {

constexpr const char* kSystemMenuBind = "bind_live_system_menu";
constexpr const char* kUnattendedInstallVariable = "LIVE_RECOMP_INSTALL_ISO";
constexpr const char* kPrimaryGpuPlugin = "xenos";

std::unique_ptr<rex::system::IInputSystem> CreateSharedControllerInput(bool tool_mode) {
  auto input = rex::input::CreateDefaultInputSystem(tool_mode);
  input->SetDeviceAssignment(std::make_unique<rex::input::SharedAssignment>());
  return input;
}

}

LiveRecompApp::LiveRecompApp(rex::ui::WindowedAppContext& context, GameDescriptor descriptor,
                             rex::PPCImageInfo image_info)
    : rex::ReXApp(context, descriptor.app_name, image_info),
      descriptor_(std::move(descriptor)),
      image_info_(image_info) {}

void LiveRecompApp::OnConfigurePaths(rex::PathConfig& paths) {
  paths_.Configure(paths);
}

void LiveRecompApp::OnPostInitLogging() {
  UserSettingsStore(paths_.settings_file()).Load();
  gaming_runtime_.Begin();
}

void LiveRecompApp::OnPreSetup(rex::RuntimeConfig& config) {
  if (config.gpu_plugin.empty()) {
    config.gpu_plugin = kPrimaryGpuPlugin;
  }
  if (REXCVAR_GET(live_shared_controllers)) {
    config.input_factory = CreateSharedControllerInput;
  }
}

void LiveRecompApp::OnConfigureStyle(ImGuiStyle& imgui_style, rex::ui::Style& overlay_style) {
  MonochromeTheme::Apply(imgui_style, overlay_style);
}

void LiveRecompApp::OnCreateDialogs(rex::ui::ImGuiDrawer*) {
  if (auto* game_window = window()) {
    game_window->SetTitle(descriptor_.display_name);
    game_window->SetCursorVisibility(rex::ui::Window::CursorVisibility::kAutoHidden);
  }
  rex::ui::RegisterBind(kSystemMenuBind, "Escape", "Open the system menu", [this] { ToggleSystemMenu(); });
}

std::optional<rex::PathConfig> LiveRecompApp::OnFinalizePaths(const rex::PathConfig& defaults,
                                                              std::function<void(rex::PathConfig)> resume) {
  rex::PathConfig paths = defaults;
  paths.game_data_root = paths_.ResolveGameRoot(defaults.game_data_root, descriptor_);
  game_data_root_ = paths.game_data_root;

  if (DiscImageInstaller::IsGameInstalled(game_data_root_)) {
    return paths;
  }
  REXLOG_INFO("Game files not found at {}", game_data_root_.string());
  if (std::getenv(kUnattendedInstallVariable)) {
    if (!InstallFromEnvironment(game_data_root_)) {
      app_context().QuitFromUIThread();
      return std::nullopt;
    }
    return paths;
  }

  DiscInstallDialog::Show(imgui_drawer(), app_context(),
                          DiscInstallRequest{
                              .game_display_name = descriptor_.display_name,
                              .install_folder = game_data_root_,
                              .owner_window = window() ? window()->GetNativeWindowHandle() : nullptr,
                              .on_installed = [paths, resume = std::move(resume)]() mutable {
                                resume(std::move(paths));
                              },
                              .on_quit = [this] { app_context().QuitFromUIThread(); },
                          });
  return std::nullopt;
}

void LiveRecompApp::OnPostLoadXexImage() {
  GuestImageDump::WriteAndExitIfRequested(*runtime(), image_info_);
}

void LiveRecompApp::OnPostSetup() {
  menu_watcher_.Start(static_cast<rex::input::InputSystem*>(runtime()->input_system()), &app_context(),
                      [this] { OpenSystemMenu(); });
}

void LiveRecompApp::OnShutdown() {
  menu_watcher_.Stop();
  gaming_runtime_.End();
  rex::ui::UnregisterBind(kSystemMenuBind);
}

bool LiveRecompApp::InstallFromEnvironment(const std::filesystem::path& game_root) {
  InstallProgress progress;
  DiscImageInstaller installer;
  if (installer.Install(std::getenv(kUnattendedInstallVariable), game_root, progress)) {
    return true;
  }
  REXLOG_ERROR("Unattended install failed: {}", installer.error());
  return false;
}

void LiveRecompApp::ToggleSystemMenu() {
  if (settings_dialog_) {
    settings_dialog_->RequestClose();
  } else if (system_menu_) {
    system_menu_->RequestClose();
  } else {
    OpenSystemMenu();
  }
}

void LiveRecompApp::OpenSystemMenu() {
  if (system_menu_ || settings_dialog_) {
    return;
  }
  system_menu_ = new SystemMenuDialog(
      imgui_drawer(), SystemMenuActions{
                          .game_display_name = descriptor_.display_name,
                          .open_settings = [this] { OpenSettings(); },
                          .exit_game =
                              [this] {
                                if (auto* game_window = window()) {
                                  game_window->RequestClose();
                                }
                              },
                          .on_closed = [this] { system_menu_ = nullptr; },
                      });
}

void LiveRecompApp::OpenSettings() {
  if (settings_dialog_) {
    return;
  }
  settings_dialog_ = new SettingsDialog(
      imgui_drawer(), SettingsContext{
                          .settings_file = paths_.settings_file(),
                          .game_data_root = game_data_root_,
                          .user_data_root = paths_.user_data_root(),
                          .portable = paths_.portable(),
                          .apply_fullscreen =
                              [this](bool fullscreen) {
                                if (auto* game_window = window()) {
                                  game_window->SetFullscreen(fullscreen);
                                }
                              },
                          .on_closed = [this] { settings_dialog_ = nullptr; },
                      });
}

}
