// overg - ReXGlue Recompiled Project

#include "generated/default/overg_init.h"

#include "overg_app.h"
#include <rex/logging.h>
#include <rex/ppc/context.h>

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
