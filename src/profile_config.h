#pragma once

#include <filesystem>
#include <random>
#include <string>
#include <system_error>
#ifdef _WIN32
#include <windows.h>
#endif

inline std::filesystem::path ProfileConfigPath(const std::filesystem::path& profile,
                                              const std::filesystem::path& legacy) {
  if (profile.empty()) return legacy;
  const auto destination = profile / "overg.toml";
  if (std::filesystem::exists(destination)) return destination;
  std::filesystem::create_directories(profile);
  if (!std::filesystem::is_regular_file(legacy)) return destination;

  // Publish only a complete copy. A concurrent first launch or a manually
  // created profile config wins if it reaches the destination first.
  std::random_device random;
  std::filesystem::path temporary;
  do {
    temporary = profile / ("overg-" + std::to_string(random()) + ".tmp");
  } while (!std::filesystem::copy_file(legacy, temporary, std::filesystem::copy_options::skip_existing));
  std::error_code error;
  std::filesystem::permissions(temporary, std::filesystem::perms::owner_write,
                               std::filesystem::perm_options::add, error);
  if (!error) {
#ifdef _WIN32
    if (!MoveFileW(temporary.c_str(), destination.c_str())) {
      error = {static_cast<int>(GetLastError()), std::system_category()};
    }
#else
    std::filesystem::create_hard_link(temporary, destination, error);
#endif
  }
  std::error_code cleanup;
  std::filesystem::remove(temporary, cleanup);
  if (error && !std::filesystem::is_regular_file(destination)) {
    throw std::filesystem::filesystem_error("Could not initialize the player configuration", destination, error);
  }
  return destination;
}
