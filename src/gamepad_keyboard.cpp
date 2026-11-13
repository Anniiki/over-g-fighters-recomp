#include "gamepad_keyboard.h"

#include <imgui_internal.h>
#include <rex/input/input.h>

#include <algorithm>
#include <array>
#include <string_view>
#include <utility>

namespace {
constexpr std::array<std::string_view, 5> kRows = {
    "ABCDEFGH", "IJKLMNOP", "QRSTUVWX", "YZ012345", "6789 -._"};
constexpr int kColumns = 8;

char KeyAt(int row, int column, bool uppercase) {
  char key = kRows[row][column];
  return !uppercase && key >= 'A' && key <= 'Z' ? char(key + ('a' - 'A')) : key;
}

// XAM allocates one extra UTF-8 byte for the editor and reserves the final
// UTF-16 unit in the guest result for its terminator. A pad key needs one unit.
bool HasRoomForKey(const ImGuiInputTextState& editor) {
  // Any multibyte text already uses fewer UTF-16 units than editor bytes.
  // ImGui enforces the byte limit for that case and selection replacement.
  return editor.TextLen < editor.BufCapacity - 2 || editor.HasSelection() ||
         std::any_of(editor.TextA.Data, editor.TextA.Data + editor.TextLen,
                     [](unsigned char byte) { return byte >= 0x80; });
}
}  // namespace

void GamepadKeyboard::Install(ImGuiContext* context, ReadButtons read_buttons) {
  read_buttons_ = std::move(read_buttons);
  ImGuiContextHook hook;
  hook.Type = ImGuiContextHookType_EndFramePre;
  hook.UserData = this;
  hook.Callback = [](ImGuiContext* ctx, ImGuiContextHook* hook) {
    static_cast<GamepadKeyboard*>(hook->UserData)->Draw(*ctx);
  };
  ImGui::AddContextHook(context, &hook);
}

void GamepadKeyboard::Draw(ImGuiContext& context) {
  // The 0.10.1 XAM keyboard uses this field in all title languages. Append
  // controls to its existing modal so XAM still owns editing and completion.
  auto* modal = ImGui::GetTopMostPopupModal();
  if (!modal || !modal->Active || context.InputTextState.ID != modal->GetID("##body") ||
      !(context.InputTextState.Flags & ImGuiInputTextFlags_EnterReturnsTrue)) {
    return;
  }
  const int open_frame = context.OpenPopupStack.back().OpenFrameCount;
  if (open_frame_ != open_frame) {
    // A later modal may reuse a title. Only its own focused text field can
    // start a new session; ImGui retains the previous editor state.
    if (context.ActiveId != context.InputTextState.ID) {
      return;
    }
    open_frame_ = open_frame;
    previous_buttons_.reset();
    pending_key_ = 0;
    column_ = row_ = 0;
    uppercase_ = true;
  }
  const auto buttons = read_buttons_();
  if (!buttons) {
    previous_buttons_.reset();
    pending_key_ = 0;
    return;
  }
  uint16_t pressed = *buttons & ~previous_buttons_.value_or(*buttons);
  previous_buttons_ = buttons;
  // When keyboard-to-pad emulation is enabled, a physical key may also
  // appear as a pad button. Give the text editor the original keyboard event.
  for (int key = ImGuiKey_Tab; key <= ImGuiKey_AppForward; ++key) {
    if (ImGui::IsKeyDown(static_cast<ImGuiKey>(key))) {
      pressed = 0;
      break;
    }
  }
  if (!ImGui::BeginPopupModal(modal->Name, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    return;
  }
  using namespace rex::input;
  constexpr int rows = int(kRows.size());
  column_ = (column_ + kColumns + bool(pressed & X_INPUT_GAMEPAD_DPAD_RIGHT) -
             bool(pressed & X_INPUT_GAMEPAD_DPAD_LEFT)) % kColumns;
  row_ = (row_ + rows + bool(pressed & X_INPUT_GAMEPAD_DPAD_DOWN) -
          bool(pressed & X_INPUT_GAMEPAD_DPAD_UP)) % rows;
  if (pressed & X_INPUT_GAMEPAD_Y) {
    uppercase_ = !uppercase_;
  }
  if (pressed & X_INPUT_GAMEPAD_B) {
    pending_key_ = 0;
    ImGui::ActivateItemByID(modal->GetID("Cancel"));
  } else if (pressed & X_INPUT_GAMEPAD_START) {
    pending_key_ = 0;
    ImGui::ActivateItemByID(modal->GetID("OK"));
  } else if (pressed & (X_INPUT_GAMEPAD_A | X_INPUT_GAMEPAD_X)) {
    pending_key_ = (pressed & X_INPUT_GAMEPAD_X) ? '\b' : KeyAt(row_, column_, uppercase_);
    if (context.ActiveId != context.InputTextState.ID) {
      ImGui::ActivateItemByID(context.InputTextState.ID);
      context.NavNextActivateFlags |= ImGuiActivateFlags_TryToPreserveState;
    }
  }
  if (pending_key_ && context.ActiveId == context.InputTextState.ID) {
    if (pending_key_ == '\b') {
      context.IO.AddKeyEvent(ImGuiKey_Backspace, true);
      context.IO.AddKeyEvent(ImGuiKey_Backspace, false);
    } else if (HasRoomForKey(context.InputTextState)) {
      context.IO.AddInputCharacter(pending_key_);
    }
    pending_key_ = 0;
  }

  ImGui::Separator();
  if (ImGui::BeginTable("##gamepad_keys", kColumns, ImGuiTableFlags_SizingStretchSame)) {
    for (int row = 0; row < rows; ++row) {
      ImGui::TableNextRow();
      for (int column = 0; column < kColumns; ++column) {
        ImGui::TableSetColumnIndex(column);
        if (row == row_ && column == column_) {
          ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,
                                 ImGui::GetColorU32(ImGuiCol_TextSelectedBg));
        }
        char label[] = {KeyAt(row, column, uppercase_), '\0'};
        ImGui::TextUnformatted(label[0] == ' ' ? "SP" : label);
      }
    }
    ImGui::EndTable();
  }
  ImGui::TextUnformatted("D-pad: Move    A: Type    X: Delete\nY: Shift    Start: OK    B: Cancel");
  ImGui::EndPopup();
}
