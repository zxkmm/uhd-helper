#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "profile_util.hpp"

namespace uhd_helper {

struct AppConfig {
  int schema_version = 1;
  std::filesystem::path uhd_dir;
  std::string images_folder_name;
  std::string idle_profile_prefix;
  std::string official_profile_folder;
  std::string backup_profile_folder;
  std::string active_profile_id;
  std::vector<Profile> profiles;
};

class ConfigManager {
 public:
  explicit ConfigManager(std::filesystem::path path);

  bool Load(std::string* error);
  bool Save(std::string* error) const;

  AppConfig& config() { return config_; }
  const AppConfig& config() const { return config_; }
  const std::filesystem::path& path() const { return path_; }

 private:
  std::filesystem::path path_;
  AppConfig config_;
};

std::filesystem::path DefaultConfigPath();
Profile* FindProfileById(AppConfig& config, const std::string& id);
const Profile* FindProfileById(const AppConfig& config, const std::string& id);
void EnsureOfficialProfile(AppConfig& config);
void NormalizeProfiles(AppConfig& config);

}  // namespace uhd_helper
