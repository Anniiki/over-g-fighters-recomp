#pragma once

#include <imgui_internal.h>
#include <rex/ui/overlay/achievement_toast.h>
#include <rex/ui/windowed_app_context.h>

#include <memory>
#include <mutex>

class AchievementToasts : public std::enable_shared_from_this<AchievementToasts> {
 public:
  AchievementToasts(rex::ui::WindowedAppContext& context, rex::ui::ImGuiDrawer* drawer,
                    rex::ui::ImmediateDrawer* immediate, rex::Runtime* runtime)
      : context_(&context), drawer_(drawer), immediate_(immediate), runtime_(runtime) {}

  void Notify(const rex::system::AchievementEvent& event) {
    std::lock_guard lock(mutex_);
    if (!context_) return;
    context_->CallInUIThreadDeferred([self = shared_from_this(), event] {
      if (!self->drawer_) return;
      if (!self->dialog_) {
        self->dialog_ = std::make_unique<Dialog>(self->drawer_, self->immediate_, self->runtime_);
      }
      self->drawer_->AddDialog(self->dialog_.get());
      self->dialog_->Push(event);
    });
  }

  void StopOnUIThread() {
    std::lock_guard lock(mutex_);
    context_ = nullptr;
    drawer_ = nullptr;
    dialog_.reset();
  }

 private:
  class Dialog final : public rex::ui::AchievementToastDialog {
   public:
    using rex::ui::AchievementToastDialog::AchievementToastDialog;

   protected:
    void OnDraw(ImGuiIO& io) override {
      // The SDK submits one ImGui window while the front toast is visible and
      // submits none once its queue drains. Keep its timing and rendering intact.
      const int before = io.Ctx->WindowsActiveCount;
      AchievementToastDialog::OnDraw(io);
      if (io.Ctx->WindowsActiveCount == before) {
        imgui_drawer()->RemoveDialog(this);
      }
    }
  };

  std::mutex mutex_;
  rex::ui::WindowedAppContext* context_;
  // These objects are used only by the UI thread.
  rex::ui::ImGuiDrawer* drawer_;
  rex::ui::ImmediateDrawer* immediate_;
  rex::Runtime* runtime_;
  std::unique_ptr<Dialog> dialog_;
};
