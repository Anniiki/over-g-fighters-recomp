#pragma once

#include <windows.h>
#include <bcrypt.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string_view>

#pragma comment(lib, "bcrypt.lib")

// The translation was generated for one retail executable. Validate it once
// at startup, before the SDK maps the disc image into the guest address space.
inline std::optional<std::string_view> RetailDiscError(const std::filesystem::path& root) {
  constexpr ULONG size = 7454720;
  constexpr std::array<UCHAR, 32> expected = {
      0x51, 0xcc, 0x8b, 0x46, 0x17, 0x0e, 0x7e, 0x96, 0x5a, 0x9a, 0x46, 0x55, 0x72,
      0x7c, 0x98, 0xe7, 0xc9, 0xe8, 0x3a, 0xbc, 0x73, 0xa7, 0x0a, 0xae, 0x8c, 0xc1,
      0x56, 0xd2, 0x6e, 0xde, 0xf4, 0x53};
  constexpr std::string_view missing =
      "Could not open default.xex. Put the extracted disc in the game folder beside overg.exe "
      "or use --game_data_root to select the extracted disc folder.";
  constexpr std::string_view mismatch =
      "This copy of default.xex does not match the supported USA/Europe disc. "
      "Check the SHA-256 fingerprint in the project README.";
  if (root.empty()) return missing;

  std::ifstream file(root / "default.xex", std::ios::binary | std::ios::ate);
  if (!file) return missing;
  if (file.tellg() != static_cast<std::streamoff>(size)) return mismatch;
  auto bytes = std::make_unique_for_overwrite<UCHAR[]>(size);
  file.seekg(0);
  if (!file.read(reinterpret_cast<char*>(bytes.get()), size))
    return "Could not read default.xex. Check that the extracted file is accessible.";
  std::array<UCHAR, expected.size()> digest{};
  if (!BCRYPT_SUCCESS(BCryptHash(BCRYPT_SHA256_ALG_HANDLE, nullptr, 0, bytes.get(), size,
                                 digest.data(), static_cast<ULONG>(digest.size()))))
    return "Windows could not calculate the SHA-256 fingerprint of default.xex.";
  if (digest != expected) return mismatch;
  return std::nullopt;
}
