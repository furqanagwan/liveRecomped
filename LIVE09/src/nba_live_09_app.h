#pragma once

#include <memory>

#include "live/app/live_recomp_app.h"

class NbaLive09App final : public live::LiveRecompApp {
 public:
  static std::unique_ptr<rex::ui::WindowedApp> Create(rex::ui::WindowedAppContext& context) {
    return std::unique_ptr<NbaLive09App>(new NbaLive09App(context));
  }

 private:
  explicit NbaLive09App(rex::ui::WindowedAppContext& context)
      : LiveRecompApp(context, Descriptor(), PPCImageConfig) {}

  static live::GameDescriptor Descriptor() {
    live::GameDescriptor descriptor;
    descriptor.app_name = "nba_live_09";
    descriptor.display_name = "NBA LIVE 09";
#ifdef LIVE_DEVELOPMENT_GAME_ROOT
    descriptor.development_game_root = LIVE_DEVELOPMENT_GAME_ROOT;
#endif
    return descriptor;
  }
};
