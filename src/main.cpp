// overg - ReXGlue Recompiled Project

#include "generated/default/overg_init.h"

#include "overg_app.h"
#include <rex/logging.h>

void OvergTraceGuestEntry() {
  REXLOG_INFO("Over G Fighters entered recompiled guest code at 0x8235EBA8");
}

REX_DEFINE_APP(overg, OvergApp::Create)
