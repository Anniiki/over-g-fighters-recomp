#include <algorithm>
#include <array>
#include <cstdio>
#include <stdexcept>

#include <rex/graphics/util/draw.h>
#include <rex/system/xmemory.h>

namespace gpu = rex::graphics;

namespace {
void Check(bool value, const char* label) {
  if (!value) throw std::runtime_error(label);
}

struct Case {
  const char* label;
  std::array<float, 6> vertices;
  unsigned left, top, right, bottom, pitch;
  unsigned width_div_8, height_div_8;
};

constexpr Case kCases[] = {
    {"aligned zero width", {16, 8, 16, 16, 16, 8}, 0, 0, 64, 64, 64, 0, 0},
    {"unaligned zero width", {13, 8, 13, 16, 13, 8}, 0, 0, 64, 64, 64, 0, 0},
    {"unaligned zero height", {8, 13, 16, 13, 8, 13}, 0, 0, 64, 64, 64, 0, 0},
    {"fully outside scissor", {0, 0, 8, 0, 0, 40}, 13, 0, 40, 48, 64, 0, 0},
    {"outside surface pitch", {40, 0, 48, 0, 40, 24}, 0, 0, 64, 64, 32, 0, 0},
    {"reversed scissor", {0, 0, 32, 0, 0, 32}, 13, 0, 5, 64, 64, 0, 0},
    {"one covered pixel", {13, 13, 14, 13, 13, 14}, 0, 0, 64, 64, 64, 1, 1},
    {"nonempty clipped area", {0, 0, 48, 0, 0, 32}, 8, 8, 40, 24, 64, 4, 2},
};
}

int main() {
  try {
    rex::memory::Memory memory;
    Check(memory.Initialize(), "guest memory initialization");
    const auto vaddr = memory.SystemHeapAlloc(24, 32, rex::memory::kSystemHeapPhysical);
    Check(vaddr != 0, "physical vertex allocation");
    const auto phys = memory.GetPhysicalAddress(vaddr);
    Check(phys != UINT32_MAX, "physical vertex address");
    unsigned failures = 0;
    gpu::RegisterFile regs;
    gpu::xenos::xe_gpu_vertex_fetch_t fetch{};
    fetch.type = gpu::xenos::FetchConstantType::kVertex;
    fetch.address = phys / 4;
    fetch.size = 6;
    regs[gpu::XE_GPU_REG_SHADER_CONSTANT_FETCH_00_0] = fetch.dword_0;
    regs[gpu::XE_GPU_REG_SHADER_CONSTANT_FETCH_00_1] = fetch.dword_1;
    gpu::reg::PA_SU_VTX_CNTL pixel{};
    pixel.pix_center = gpu::xenos::PixelCenter::kOGLHalf;
    regs[pixel.register_index] = pixel.value;
    regs[gpu::XE_GPU_REG_PA_SC_SCREEN_SCISSOR_BR] = (64 << 16) | 64;
    regs[gpu::XE_GPU_REG_RB_COPY_DEST_PITCH] = (64 << 16) | 64;
    gpu::reg::RB_COPY_DEST_INFO dest{};
    dest.copy_dest_format = gpu::xenos::ColorFormat::k_8_8_8_8;
    regs[dest.register_index] = dest.value;
    regs[gpu::XE_GPU_REG_RB_COLOR_CLEAR] = 0x11223344;
    regs[gpu::XE_GPU_REG_RB_DEPTH_CLEAR] = 0xdead00ff;

    for (const auto& test : kCases) {
      std::copy(test.vertices.begin(), test.vertices.end(), memory.TranslatePhysical<float*>(phys));
      regs[gpu::XE_GPU_REG_RB_SURFACE_INFO] = test.pitch;
      regs[gpu::XE_GPU_REG_PA_SC_WINDOW_SCISSOR_TL] = test.left | (test.top << 16);
      regs[gpu::XE_GPU_REG_PA_SC_WINDOW_SCISSOR_BR] = test.right | (test.bottom << 16);
      bool geometry_ok = true;
      for (const unsigned source : {0u, 4u}) {
        for (const auto command : {gpu::xenos::CopyCommand::kRaw, gpu::xenos::CopyCommand::kConvert}) {
          for (unsigned flags = 0; flags < 4; ++flags) {
            gpu::reg::RB_COPY_CONTROL control{};
            control.copy_src_select = source;
            control.copy_command = command;
            control.color_clear_enable = flags & 1;
            control.depth_clear_enable = flags >> 1;
            regs[control.register_index] = control.value;
            gpu::draw_util::ResolveInfo info{};
            info.coordinate_info.packed = UINT32_MAX;
            info.height_div_8 = UINT32_MAX;
            const bool accepted = gpu::draw_util::GetResolveInfo(regs, memory, 1, 1, false, false, info);
            if (!accepted || info.coordinate_info.width_div_8 != test.width_div_8 ||
                info.height_div_8 != test.height_div_8) {
              if (geometry_ok) {
                std::fprintf(stderr, "FAIL %s (accepted=%d, extent=%u x %u blocks)\n",
                             test.label, accepted, unsigned(info.coordinate_info.width_div_8),
                             info.height_div_8);
              }
              geometry_ok = false;
              ++failures;
              continue;
            }
            Check(info.rb_copy_control.value == control.value, "copy control");
            if (test.width_div_8 && test.height_div_8) {
              Check(info.copy_dest_extent_length != 0, "nonempty copy");
              Check(info.IsClearingColor() == (source == 0 && (flags & 1)), "color clear");
              Check(info.IsClearingDepth() == bool(flags & 2), "depth clear");
              Check(info.rb_color_clear == 0x11223344 && info.rb_depth_clear == 0xdead00ff,
                    "clear values");
            }
          }
        }
      }
      if (geometry_ok) std::printf("PASS %s\n", test.label);
    }
    gpu::draw_util::ResolveInfo invalid{};
    regs[gpu::XE_GPU_REG_SHADER_CONSTANT_FETCH_00_1] = 0;
    Check(!gpu::draw_util::GetResolveInfo(regs, memory, 1, 1, false, false, invalid),
          "invalid vertex fetch must fail");
    regs[gpu::XE_GPU_REG_SHADER_CONSTANT_FETCH_00_1] = fetch.dword_1;
    regs[gpu::XE_GPU_REG_RB_SURFACE_INFO] |= 3 << 16;
    regs[gpu::XE_GPU_REG_PA_SC_WINDOW_SCISSOR_BR] = 0;
    Check(!gpu::draw_util::GetResolveInfo(regs, memory, 1, 1, false, false, invalid),
          "invalid MSAA must fail even for empty coverage");
    memory.SystemHeapFree(vaddr);
    std::puts("PASS malformed packets");
    if (failures) std::fprintf(stderr, "%u geometry combinations failed\n", failures);
    return failures ? 1 : 0;
  } catch (const std::exception& e) {
    std::fprintf(stderr, "FAIL %s\n", e.what());
    return 1;
  }
}
