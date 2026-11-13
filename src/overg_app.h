// overg - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <filesystem>
#include <rex/rex_app.h>

#include "achievement_toasts.h"
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
  void OnConfigurePaths(rex::PathConfig& paths) override {
    if (paths.user_data_root.empty()) {
      return;
    }
    const auto player_config = paths.user_data_root / "overg.toml";
    if (std::filesystem::exists(player_config) || !std::filesystem::exists(paths.config_path)) {
      paths.config_path = player_config;
    }
  }

  void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override;
  std::unique_ptr<rex::ui::AchievementNotificationDialog> CreateAchievementNotificationDialog() override;
  void OnShutdown() override;

  void OnPreSetup(rex::RuntimeConfig& config) override {
    if (config.gpu_plugin.empty()) {
      config.gpu_plugin = "xenos";
    }
  }

 private:
  GamepadKeyboard keyboard_;
  std::shared_ptr<AchievementToasts> toasts_;
  rex::system::AchievementListenerHandle toast_listener_ = 0;
};
