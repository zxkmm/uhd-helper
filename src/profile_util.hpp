#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace uhd_helper {

struct Profile {
  std::string id;
  std::string display_name;
  std::string folder_name;
  bool is_official = false;
};

class ConfigManager;

class ProfileManager {
 public:
  explicit ProfileManager(ConfigManager* config_manager);

  bool Initialize(std::string* error);
  bool ApplyProfile(const std::string& profile_id, std::string* error);
  bool AddProfileFromActive(const std::string& display_name,
                            std::string* error);
  bool DeleteProfile(const std::string& profile_id, std::string* error);
  bool ResetToOfficial(std::string* error);
  bool RefreshFromDisk(std::string* error);

  const std::vector<Profile>& Profiles() const;
  std::string ActiveProfileId() const;
  std::filesystem::path UhdDir() const;
  std::filesystem::path ImagesPath() const;
  std::filesystem::path ConfigPath() const;

 private:
  std::string GenerateProfileId(const std::string& display_name) const;
  bool EnsureUhdDir(std::string* error) const;
  bool RenameActiveToIdle(std::string* error);

  ConfigManager* config_manager_;
};

}  // namespace uhd_helper
