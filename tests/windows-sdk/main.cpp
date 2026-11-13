#include <rex/cvar.h>
#include <rex/ppc/context.h>
#include "../../src/profile_config.h"

#include <cstdio>
#include <string_view>

REX_EXTERN(__imp__XGetLanguage);

int main(int argc, char** argv) {
    rex::cvar::Init(argc, argv);
    // Use the same config path for SDK startup and its "Save to config" action.
    if (argc >= 4 && std::string_view(argv[1]) == "--profile-check") {
        const auto config = ProfileConfigPath(std::filesystem::path(argv[2]), std::filesystem::path(argv[3]));
        rex::cvar::LoadConfig(config);
        if (argc == 5 && !std::string_view(argv[4]).starts_with("--")) {
            if (!rex::cvar::SetFlagByName("user_language", argv[4])) return 1;
            rex::cvar::SaveConfig(config);
        }
    }
    PPCContext context{};
    __imp__XGetLanguage(context, nullptr);
    std::printf("%llu\n", static_cast<unsigned long long>(context.r3.u64));
}
