#pragma once

#include <cstdint>
#include <functional>
#include <optional>

struct ImGuiContext;

class GamepadKeyboard {
 public:
  using ReadButtons = std::function<std::optional<uint16_t>()>;
  void Install(ImGuiContext* context, ReadButtons read_buttons);

 private:
  void Draw(ImGuiContext& context);
  ReadButtons read_buttons_;
  int open_frame_ = -1;
  int column_ = 0;
  int row_ = 0;
  bool uppercase_ = true;
  char pending_key_ = 0;
  std::optional<uint16_t> previous_buttons_;
};
