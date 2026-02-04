#include "res.hpp"

#include <cstdlib>

namespace uhd_helper {

Os DetectOs() {
#if defined(__linux__)
  return Os::kLinux;
#else
  return Os::kUnknown;
#endif
}

std::filesystem::path GetUhdDirByOs(Os os) {
  switch (os) {
    case Os::kLinux:
      return std::filesystem::path("/usr/share/uhd");
    case Os::kUnknown:
      break;
  }
  return std::filesystem::path();
}

std::string GetImagesFolderName(UhdVersion /*version*/) {
  return "images";
}

const AppDefaults& Defaults() {
  static const AppDefaults defaults;
  return defaults;
}

}  // namespace uhd_helper
