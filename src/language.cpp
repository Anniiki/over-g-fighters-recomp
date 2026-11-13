#include <rex/ppc/context.h>
#include <rex/system/flags.h>

// ReXGlue 0.10.1 returns English for this XAM import. Use the same setting
// as the kernel's XCONFIG_USER_LANGUAGE query.
extern "C" REX_FUNC(__imp__XGetLanguage) {
  ctx.r3.u64 = REXCVAR_GET(user_language);
}
