#include "profile_util.hpp"

#include <algorithm>
#include <cctype>
#include <unordered_set>

#include "config_util.hpp"
#include "file_util.hpp"
#include "res.hpp"

namespace uhd_helper {
namespace {

std::string ToLowerAscii(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}

std::string Slugify(const std::string& input) {
  std::string out;
  out.reserve(input.size());
  for (unsigned char c : input) {
    if (std::isalnum(c)) {
      out.push_back(static_cast<char>(std::tolower(c)));
    } else if (c == ' ' || c == '-' || c == '_') {
      if (!out.empty() && out.back() != '_') {
        out.push_back('_');
      }
    }
  }
  if (!out.empty() && out.back() == '_') {
    out.pop_back();
  }
  return out;
}

bool FolderExists(const std::filesystem::path& path) {
  return FileUtil::Exists(path) && FileUtil::IsDir(path);
}

}  // namespace

ProfileManager::ProfileManager(ConfigManager* config_manager)
    : config_manager_(config_manager) {}

bool ProfileManager::Initialize(std::string* error) {
  if (!config_manager_) {
    if (error) {
      *error = "Config manager is not set";
    }
    return false;
  }
  if (!config_manager_->Load(error)) {
    return false;
  }
  return RefreshFromDisk(error);
}

const std::vector<Profile>& ProfileManager::Profiles() const {
  return config_manager_->config().profiles;
}

std::string ProfileManager::ActiveProfileId() const {
  return config_manager_->config().active_profile_id;
}

std::filesystem::path ProfileManager::UhdDir() const {
  return config_manager_->config().uhd_dir;
}

std::filesystem::path ProfileManager::ImagesPath() const {
  const auto& cfg = config_manager_->config();
  return cfg.uhd_dir / cfg.images_folder_name;
}

std::filesystem::path ProfileManager::ConfigPath() const {
  return config_manager_->path();
}

bool ProfileManager::EnsureUhdDir(std::string* error) const {
  return FileUtil::EnsureDir(config_manager_->config().uhd_dir, error);
}

std::string ProfileManager::GenerateProfileId(
    const std::string& display_name) const {
  const auto& config = config_manager_->config();
  std::string base = Slugify(display_name);
  if (base.empty()) {
    base = "profile";
  }

  std::unordered_set<std::string> existing;
  for (const auto& profile : config.profiles) {
    existing.insert(profile.id);
  }

  if (existing.count(base) == 0) {
    return base;
  }
  for (int i = 2; i < 10000; ++i) {
    std::string candidate = base + "_" + std::to_string(i);
    if (existing.count(candidate) == 0) {
      return candidate;
    }
  }
  return base + "_x";
}

bool ProfileManager::RenameActiveToIdle(std::string* error) {
  auto& cfg = config_manager_->config();
  const std::filesystem::path images_path = ImagesPath();
  if (!FolderExists(images_path)) {
    return true;
  }

  if (!cfg.active_profile_id.empty()) {
    Profile* active = FindProfileById(cfg, cfg.active_profile_id);
    if (active && !active->folder_name.empty()) {
      const auto dest = cfg.uhd_dir / active->folder_name;
      if (FolderExists(dest)) {
        if (!FileUtil::RemoveAll(dest, error)) {
          return false;
        }
      }
      return FileUtil::Rename(images_path, dest, error);
    }
  }

  const auto backup_dest = cfg.uhd_dir / cfg.backup_profile_folder;
  if (FolderExists(backup_dest)) {
    FileUtil::RemoveAll(backup_dest, nullptr);
  }
  return FileUtil::Rename(images_path, backup_dest, error);
}

bool ProfileManager::ApplyProfile(const std::string& profile_id,
                                  std::string* error) {
  if (!EnsureUhdDir(error)) {
    return false;
  }

  auto& cfg = config_manager_->config();
  Profile* target = FindProfileById(cfg, profile_id);
  if (!target) {
    if (error) {
      *error = "Unknown profile id: " + profile_id;
    }
    return false;
  }

  const auto target_path = cfg.uhd_dir / target->folder_name;
  if (!FolderExists(target_path)) {
    if (profile_id == cfg.active_profile_id &&
        FolderExists(ImagesPath())) {
      return true;
    }
    if (error) {
      *error = "Profile folder does not exist: " + target_path.string();
    }
    return false;
  }

  if (!RenameActiveToIdle(error)) {
    return false;
  }

  const auto images_path = ImagesPath();
  if (!FileUtil::Rename(target_path, images_path, error)) {
    return false;
  }

  cfg.active_profile_id = target->id;
  return config_manager_->Save(error);
}

bool ProfileManager::AddProfileFromActive(const std::string& display_name,
                                          std::string* error) {
  if (!EnsureUhdDir(error)) {
    return false;
  }

  const auto& cfg = config_manager_->config();
  Profile* official = FindProfileById(config_manager_->config(), "official");
  if (!official) {
    if (error) {
      *error = "Official profile is missing";
    }
    return false;
  }

  std::filesystem::path source_path = cfg.uhd_dir / official->folder_name;
  if (!FolderExists(source_path)) {
    if (cfg.active_profile_id == "official" && FolderExists(ImagesPath())) {
      source_path = ImagesPath();
    } else {
      if (error) {
        *error = "Official profile folder does not exist: " +
                 source_path.string();
      }
      return false;
    }
  }

  Profile profile;
  profile.id = GenerateProfileId(display_name);
  profile.display_name = display_name.empty() ? profile.id : display_name;
  profile.folder_name = cfg.idle_profile_prefix + profile.id;
  profile.is_official = false;

  const auto dest = cfg.uhd_dir / profile.folder_name;
  if (FolderExists(dest)) {
    if (error) {
      *error = "Profile folder already exists: " + dest.string();
    }
    return false;
  }

  if (!FileUtil::CopyDir(source_path, dest, error)) {
    return false;
  }

  config_manager_->config().profiles.push_back(profile);
  return config_manager_->Save(error);
}

bool ProfileManager::DeleteProfile(const std::string& profile_id,
                                   std::string* error) {
  auto& cfg = config_manager_->config();
  if (profile_id.empty()) {
    if (error) {
      *error = "Profile id is empty";
    }
    return false;
  }
  if (profile_id == cfg.active_profile_id) {
    if (error) {
      *error = "Cannot delete the active profile";
    }
    return false;
  }

  auto it = std::find_if(cfg.profiles.begin(), cfg.profiles.end(),
                         [&](const Profile& p) { return p.id == profile_id; });
  if (it == cfg.profiles.end()) {
    if (error) {
      *error = "Profile not found";
    }
    return false;
  }
  if (it->is_official) {
    if (error) {
      *error = "Cannot delete the official profile";
    }
    return false;
  }

  const auto target_path = cfg.uhd_dir / it->folder_name;
  if (FolderExists(target_path)) {
    if (!FileUtil::RemoveAll(target_path, error)) {
      return false;
    }
  }

  cfg.profiles.erase(it);
  return config_manager_->Save(error);
}

bool ProfileManager::ResetToOfficial(std::string* error) {
  return ApplyProfile("official", error);
}

bool ProfileManager::RefreshFromDisk(std::string* error) {
  if (!EnsureUhdDir(error)) {
    return false;
  }

  auto& cfg = config_manager_->config();
  const auto official_path = cfg.uhd_dir / cfg.official_profile_folder;
  if (!FolderExists(official_path) && FolderExists(ImagesPath())) {
    if (!FileUtil::CopyDir(ImagesPath(), official_path, error)) {
      return false;
    }
  }

  std::unordered_set<std::string> known_folders;
  for (const auto& profile : cfg.profiles) {
    known_folders.insert(profile.folder_name);
  }

  const auto dirs = FileUtil::ListDirs(cfg.uhd_dir);
  for (const auto& dir : dirs) {
    const std::string name = dir.filename().string();
    if (name == cfg.images_folder_name) {
      continue;
    }
    if (known_folders.count(name) > 0) {
      continue;
    }
    if (name == cfg.backup_profile_folder) {
      continue;
    }
    if (name.rfind(cfg.idle_profile_prefix, 0) != 0) {
      continue;
    }
    Profile profile;
    profile.folder_name = name;
    profile.id = ToLowerAscii(name.substr(cfg.idle_profile_prefix.size()));
    if (profile.id.empty()) {
      continue;
    }
    profile.display_name = profile.id;
    profile.is_official = false;
    cfg.profiles.push_back(std::move(profile));
  }

  NormalizeProfiles(cfg);
  return config_manager_->Save(error);
}

}  // namespace uhd_helper
