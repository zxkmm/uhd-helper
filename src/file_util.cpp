#include "file_util.hpp"

#include <system_error>

namespace uhd_helper {

bool FileUtil::EnsureDir(const std::filesystem::path& dir, std::string* error) {
  std::error_code ec;
  if (std::filesystem::exists(dir, ec)) {
    if (!std::filesystem::is_directory(dir, ec)) {
      if (error) {
        *error = "Path exists but is not a directory: " + dir.string();
      }
      return false;
    }
    return true;
  }
  if (!std::filesystem::create_directories(dir, ec)) {
    if (error) {
      *error = "Failed to create directory: " + dir.string();
    }
    return false;
  }
  return true;
}

bool FileUtil::Exists(const std::filesystem::path& path) {
  std::error_code ec;
  return std::filesystem::exists(path, ec);
}

bool FileUtil::IsDir(const std::filesystem::path& path) {
  std::error_code ec;
  return std::filesystem::is_directory(path, ec);
}

bool FileUtil::RemoveAll(const std::filesystem::path& path, std::string* error) {
  std::error_code ec;
  std::filesystem::remove_all(path, ec);
  if (ec) {
    if (error) {
      *error = "Failed to remove: " + path.string();
    }
    return false;
  }
  return true;
}

bool FileUtil::Rename(const std::filesystem::path& from,
                      const std::filesystem::path& to,
                      std::string* error) {
  std::error_code ec;
  std::filesystem::rename(from, to, ec);
  if (ec) {
    if (error) {
      *error = "Failed to rename from " + from.string() + " to " + to.string();
    }
    return false;
  }
  return true;
}

bool FileUtil::CopyDir(const std::filesystem::path& from,
                       const std::filesystem::path& to,
                       std::string* error) {
  std::error_code ec;
  if (!std::filesystem::exists(from, ec)) {
    if (error) {
      *error = "Source does not exist: " + from.string();
    }
    return false;
  }
  std::filesystem::copy(from, to,
                        std::filesystem::copy_options::recursive |
                            std::filesystem::copy_options::overwrite_existing,
                        ec);
  if (ec) {
    if (error) {
      *error = "Failed to copy from " + from.string() + " to " + to.string();
    }
    return false;
  }
  return true;
}

std::vector<std::filesystem::path> FileUtil::ListDirs(
    const std::filesystem::path& parent) {
  std::vector<std::filesystem::path> result;
  std::error_code ec;
  if (!std::filesystem::is_directory(parent, ec)) {
    return result;
  }
  for (const auto& entry : std::filesystem::directory_iterator(parent, ec)) {
    if (entry.is_directory(ec)) {
      result.push_back(entry.path());
    }
  }
  return result;
}

}  // namespace uhd_helper
