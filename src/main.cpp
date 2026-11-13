// overg - ReXGlue Recompiled Project

#include "generated/default/overg_init.h"

#include "overg_app.h"
#include <rex/input/input_system.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>

void OvergApp::OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) {
  keyboard_.Install(drawer->GetIO().Ctx, [this]() -> std::optional<uint16_t> {
    if (!runtime() || !window()->HasFocus()) {
      return std::nullopt;
    }
    auto* input = static_cast<rex::input::InputSystem*>(runtime()->input_system());
    rex::input::X_INPUT_STATE state{};
    if (!input || input->GetStateForUI(0, &state) != 0) {
      return std::nullopt;
    }
    return uint16_t(state.gamepad.buttons);
  });
}

void OvergTraceGuestEntry() {
  REXLOG_INFO("Over G Fighters entered recompiled guest code at 0x8235EBA8");
}

void OvergInitAudioFpscr(PPCFPSCRRegister& fpscr) {
  // XHostThread lacks the InitHost call used for regular guest threads in
  // ReXGlue 0.10.1. Seed its empty MXCSR cache before the first audio mix.
  constexpr auto masks = PPCFPSCRRegister::Platform::ExceptionMask;
  if ((fpscr.csr & masks) != masks) {
    fpscr.InitHost();
  }
}

REX_DEFINE_APP(overg, OvergApp::Create)
