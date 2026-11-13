#include <rex/cvar.h>
#include <rex/ppc/context.h>

#include <cstdio>

REX_EXTERN(__imp__XGetLanguage);

int main(int argc, char** argv) {
    rex::cvar::Init(argc, argv);
    PPCContext context{};
    __imp__XGetLanguage(context, nullptr);
    std::printf("%llu\n", static_cast<unsigned long long>(context.r3.u64));
}
