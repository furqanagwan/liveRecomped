#pragma once

#include <array>
#include <filesystem>
#include <functional>
#include <string>
#include <thread>

#include <rex/ui/imgui_dialog.h>

#include "live/installer/disc_image_installer.h"
#include "live/platform/native_file_picker.h"

namespace rex::ui {
class WindowedAppContext;
}

namespace live {

struct DiscInstallRequest {
  std::string game_display_name;
  std::filesystem::path install_folder;
  void* owner_window = nullptr;
  std::function<void()> on_installed;
  std::function<void()> on_quit;
};

class DiscInstallDialog final : public rex::ui::ImGuiDialog {
 public:
  static void Show(rex::ui::ImGuiDrawer* drawer, rex::ui::WindowedAppContext& app_context,
                   DiscInstallRequest request);

  ~DiscInstallDialog() override;

 protected:
  void OnDraw(ImGuiIO& io) override;

 private:
  enum class Stage { kChoosingImage, kInstalling, kFailed };

  DiscInstallDialog(rex::ui::ImGuiDrawer* drawer, rex::ui::WindowedAppContext& app_context,
                    DiscInstallRequest request);

  void DrawChooseImage();
  void DrawInstalling();
  void FinishInstallIfDone();
  void BeginInstall(const std::filesystem::path& disc_image);

  rex::ui::WindowedAppContext& app_context_;
  DiscInstallRequest request_;
  NativeFilePicker file_picker_;
  Stage stage_ = Stage::kChoosingImage;
  std::array<char, 1024> typed_path_{};
  std::filesystem::path disc_image_;
  DiscImageInstaller installer_;
  InstallProgress progress_;
  std::thread worker_;
  std::atomic<bool> worker_finished_{false};
  std::atomic<bool> worker_succeeded_{false};
};

}
