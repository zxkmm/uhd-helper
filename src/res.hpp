#pragma once

#include <filesystem>
#include <string>

namespace uhd_helper {

enum class Os {
  kLinux,
  kUnknown,
};

enum class UhdVersion {
  kDefault,
};

Os DetectOs();
std::filesystem::path GetUhdDirByOs(Os os);
std::string GetImagesFolderName(UhdVersion version);

struct AppDefaults {
  std::string idle_profile_prefix = "I_P_";
  std::string official_profile_folder = "R_NI";
  std::string backup_profile_folder = "I_P__backup";
  int schema_version = 1;
};

const AppDefaults& Defaults();

}  // namespace uhd_helper
