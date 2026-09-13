#pragma once

#include <memory>

#include "live/app/live_recomp_app.h"

class @APP_CLASS@ final : public live::LiveRecompApp {
 public:
  static std::unique_ptr<rex::ui::WindowedApp> Create(rex::ui::WindowedAppContext& context) {
    return std::unique_ptr<@APP_CLASS@>(new @APP_CLASS@(context));
  }

 private:
  explicit @APP_CLASS@(rex::ui::WindowedAppContext& context)
      : LiveRecompApp(context, Descriptor(), PPCImageConfig) {}

  static live::GameDescriptor Descriptor() {
    live::GameDescriptor descriptor;
    descriptor.app_name = "@PROJECT_NAME@";
    descriptor.display_name = "@DISPLAY_NAME@";
#ifdef LIVE_DEVELOPMENT_GAME_ROOT
    descriptor.development_game_root = LIVE_DEVELOPMENT_GAME_ROOT;
#endif
    return descriptor;
  }
};
