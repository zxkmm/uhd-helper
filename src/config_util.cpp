#include "config_util.hpp"

#include <fstream>
#include <unordered_set>

#include "file_util.hpp"
#include "json_min.hpp"
#include "res.hpp"

namespace uhd_helper {
namespace {

const json_min::Value* GetObjectValue(const json_min::Object& obj,
                                      const char* key) {
  auto it = obj.find(key);
  if (it == obj.end()) {
    return nullptr;
  }
  return &it->second;
}

std::string GetString(const json_min::Object& obj, const char* key,
                      const std::string& fallback) {
  const auto* value = GetObjectValue(obj, key);
  if (!value || !value->IsString()) {
    return fallback;
  }
  return *value->AsString();
}

int GetInt(const json_min::Object& obj, const char* key, int fallback) {
  const auto* value = GetObjectValue(obj, key);
  if (!value || !value->IsNumber()) {
    return fallback;
  }
  return static_cast<int>(*value->AsNumber());
}

bool GetBool(const json_min::Object& obj, const char* key, bool fallback) {
  const auto* value = GetObjectValue(obj, key);
  if (!value || !value->IsBool()) {
    return fallback;
  }
  return *value->AsBool();
}

Profile ParseProfile(const json_min::Object& obj, const AppConfig& defaults) {
  Profile profile;
  profile.id = GetString(obj, "id", "");
  profile.display_name = GetString(obj, "display_name", profile.id);
  profile.folder_name =
      GetString(obj, "folder_name", defaults.idle_profile_prefix + profile.id);
  profile.is_official = GetBool(obj, "is_official", false);
  return profile;
}

json_min::Value ProfileToJson(const Profile& profile) {
  json_min::Object obj;
  obj.emplace("id", json_min::Value(profile.id));
  obj.emplace("display_name", json_min::Value(profile.display_name));
  obj.emplace("folder_name", json_min::Value(profile.folder_name));
  obj.emplace("is_official", json_min::Value(profile.is_official));
  return json_min::Value(std::move(obj));
}

}  // namespace

ConfigManager::ConfigManager(std::filesystem::path path) : path_(std::move(path)) {}

bool ConfigManager::Load(std::string* error) {
  std::error_code ec;
  if (!std::filesystem::exists(path_, ec)) {
    config_ = AppConfig{};
    config_.schema_version = Defaults().schema_version;
    config_.uhd_dir = GetUhdDirByOs(DetectOs());
    config_.images_folder_name = GetImagesFolderName(UhdVersion::kDefault);
    config_.idle_profile_prefix = Defaults().idle_profile_prefix;
    config_.official_profile_folder = Defaults().official_profile_folder;
    config_.backup_profile_folder = Defaults().backup_profile_folder;
    EnsureOfficialProfile(config_);
    NormalizeProfiles(config_);
    return Save(error);
  }

  std::ifstream input(path_);
  if (!input.is_open()) {
    if (error) {
      *error = "Failed to open config file: " + path_.string();
    }
    return false;
  }

  std::string content((std::istreambuf_iterator<char>(input)),
                      std::istreambuf_iterator<char>());
  json_min::Value root;
  try {
    json_min::Parser parser(std::move(content));
    root = parser.Parse();
  } catch (const std::exception& ex) {
    if (error) {
      *error = std::string("Failed to parse config JSON: ") + ex.what();
    }
    return false;
  }

  if (!root.IsObject()) {
    if (error) {
      *error = "Config JSON root is not an object";
    }
    return false;
  }

  const auto& root_obj = *root.AsObject();

  AppConfig cfg;
  cfg.schema_version =
      GetInt(root_obj, "schema_version", Defaults().schema_version);
  cfg.uhd_dir = GetString(root_obj, "uhd_dir",
                          GetUhdDirByOs(DetectOs()).string());
  cfg.images_folder_name =
      GetString(root_obj, "images_folder_name",
                GetImagesFolderName(UhdVersion::kDefault));
  cfg.idle_profile_prefix =
      GetString(root_obj, "idle_profile_prefix", Defaults().idle_profile_prefix);
  cfg.official_profile_folder =
      GetString(root_obj, "official_profile_folder",
                                          Defaults().official_profile_folder);
  cfg.backup_profile_folder = GetString(root_obj, "backup_profile_folder",
                                        Defaults().backup_profile_folder);
  cfg.active_profile_id = GetString(root_obj, "active_profile_id", "");

  const auto* profiles_value = GetObjectValue(root_obj, "profiles");
  if (profiles_value && profiles_value->IsArray()) {
    for (const auto& item : *profiles_value->AsArray()) {
      if (!item.IsObject()) {
        continue;
      }
      Profile profile = ParseProfile(*item.AsObject(), cfg);
      if (!profile.id.empty()) {
        cfg.profiles.push_back(std::move(profile));
      }
    }
  }

  config_ = std::move(cfg);
  EnsureOfficialProfile(config_);
  NormalizeProfiles(config_);
  if (config_.active_profile_id.empty()) {
    config_.active_profile_id = "official";
  }
  return true;
}

bool ConfigManager::Save(std::string* error) const {
  json_min::Object root_obj;
  root_obj.emplace("schema_version",
                   json_min::Value(static_cast<double>(config_.schema_version)));
  root_obj.emplace("uhd_dir", json_min::Value(config_.uhd_dir.string()));
  root_obj.emplace("images_folder_name",
                   json_min::Value(config_.images_folder_name));
  root_obj.emplace("idle_profile_prefix",
                   json_min::Value(config_.idle_profile_prefix));
  root_obj.emplace("official_profile_folder",
                   json_min::Value(config_.official_profile_folder));
  root_obj.emplace("backup_profile_folder",
                   json_min::Value(config_.backup_profile_folder));
  root_obj.emplace("active_profile_id",
                   json_min::Value(config_.active_profile_id));

  json_min::Array profiles;
  profiles.reserve(config_.profiles.size());
  for (const auto& profile : config_.profiles) {
    profiles.push_back(ProfileToJson(profile));
  }
  root_obj.emplace("profiles", json_min::Value(std::move(profiles)));

  std::error_code ec;
  std::filesystem::create_directories(path_.parent_path(), ec);

  std::ofstream output(path_);
  if (!output.is_open()) {
    if (error) {
      *error = "Failed to open config file for writing: " + path_.string();
    }
    return false;
  }
  json_min::Value root(std::move(root_obj));
  output << json_min::Serialize(root, 2) << '\n';
  return true;
}

std::filesystem::path DefaultConfigPath() {
  const char* xdg = std::getenv("XDG_CONFIG_HOME");
  const char* home = std::getenv("HOME");
  std::filesystem::path base;
  if (xdg && *xdg) {
    base = xdg;
  } else if (home && *home) {
    base = std::filesystem::path(home) / ".config";
  } else {
    base = std::filesystem::current_path();
  }
  return base / "uhd-helper" / "config.json";
}

Profile* FindProfileById(AppConfig& config, const std::string& id) {
  for (auto& profile : config.profiles) {
    if (profile.id == id) {
      return &profile;
    }
  }
  return nullptr;
}

const Profile* FindProfileById(const AppConfig& config, const std::string& id) {
  for (const auto& profile : config.profiles) {
    if (profile.id == id) {
      return &profile;
    }
  }
  return nullptr;
}

void EnsureOfficialProfile(AppConfig& config) {
  Profile* existing = FindProfileById(config, "official");
  if (!existing) {
    Profile official;
    official.id = "official";
    official.display_name = "NI Official";
    official.folder_name = config.official_profile_folder;
    official.is_official = true;
    config.profiles.push_back(std::move(official));
    return;
  }
  existing->folder_name = config.official_profile_folder;
  existing->is_official = true;
  if (existing->display_name.empty()) {
    existing->display_name = "NI Official";
  }
}

void NormalizeProfiles(AppConfig& config) {
  std::unordered_set<std::string> seen_ids;
  std::vector<Profile> normalized;
  normalized.reserve(config.profiles.size());

  for (auto& profile : config.profiles) {
    if (profile.id.empty()) {
      continue;
    }
    if (seen_ids.count(profile.id) > 0) {
      continue;
    }
    seen_ids.insert(profile.id);
    if (profile.display_name.empty()) {
      profile.display_name = profile.id;
    }
    if (profile.folder_name.empty()) {
      if (profile.is_official) {
        profile.folder_name = config.official_profile_folder;
      } else {
        profile.folder_name = config.idle_profile_prefix + profile.id;
      }
    }
    normalized.push_back(std::move(profile));
  }

  config.profiles = std::move(normalized);
}

}  // namespace uhd_helper
