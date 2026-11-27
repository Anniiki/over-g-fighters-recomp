// overg - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <cstdlib>
#include <filesystem>
#include <rex/rex_app.h>
#include <rex/runtime.h>

#include "achievement_toasts.h"
#include "gamepad_keyboard.h"
#include "profile_config.h"
#ifdef _WIN32
#include <rex/logging.h>
#include <rex/system.h>
#include "retail_disc.h"
#endif

class OvergApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<OvergApp>(new OvergApp(ctx, "overg",
        PPCImageConfig));
  }

 protected:
#ifdef _WIN32
  bool ConstructRuntime(const rex::PathConfig& paths) override {
    if (const auto error = RetailDiscError(paths.game_data_root)) {
      REXLOG_ERROR("{}", *error);
      rex::ShowSimpleMessageBox(rex::SimpleMessageBoxType::Error, *error);
      return false;
    }
    return rex::ReXApp::ConstructRuntime(paths);
  }
#endif

  void OnConfigurePaths(rex::PathConfig& paths) override {
    if (paths.game_data_root.empty()) {
      const auto portable = paths.config_path.parent_path() / "game";
      std::error_code error;
      if (std::filesystem::is_regular_file(portable / "default.xex", error)) {
        paths.game_data_root = portable;
      }
    }
#ifdef _WIN32
    if (REXCVAR_GET(user_data_root).empty()) {
      if (const auto* local = _wgetenv(L"LOCALAPPDATA"); local && *local) {
        const auto launcher_profile = std::filesystem::path(local) / "Over G Fighters Recomp";
        // Prefer the profile from play.cmd. Keep a standalone SDK profile when
        // it is the only one present, including its existing save slots.
        if (std::filesystem::is_directory(launcher_profile) ||
            !std::filesystem::exists(paths.user_data_root)) {
          paths.user_data_root = launcher_profile;
          if (REXCVAR_GET(cache_root).empty()) paths.cache_root = launcher_profile / "cache";
        }
      }
    }
#endif
    paths.config_path = ProfileConfigPath(paths.user_data_root, paths.config_path);
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
