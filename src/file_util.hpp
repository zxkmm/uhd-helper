#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace uhd_helper {

class FileUtil {
 public:
  static bool EnsureDir(const std::filesystem::path& dir, std::string* error);
  static bool Exists(const std::filesystem::path& path);
  static bool IsDir(const std::filesystem::path& path);
  static bool RemoveAll(const std::filesystem::path& path, std::string* error);
  static bool Rename(const std::filesystem::path& from,
                     const std::filesystem::path& to,
                     std::string* error);
  static bool CopyDir(const std::filesystem::path& from,
                      const std::filesystem::path& to,
                      std::string* error);
  static std::vector<std::filesystem::path> ListDirs(
      const std::filesystem::path& parent);
};

}  // namespace uhd_helper
