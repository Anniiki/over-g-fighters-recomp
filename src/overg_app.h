// overg - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <rex/rex_app.h>

#include "gamepad_keyboard.h"

class OvergApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<OvergApp>(new OvergApp(ctx, "overg",
        PPCImageConfig));
  }

 protected:
  void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override;

  void OnPreSetup(rex::RuntimeConfig& config) override {
    if (config.gpu_plugin.empty()) {
      config.gpu_plugin = "xenos";
    }
  }

 private:
  GamepadKeyboard keyboard_;
};
