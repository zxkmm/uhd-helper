#include <iostream>
#include <unistd.h>

#include "config_util.hpp"
#include "profile_util.hpp"
#include "tui.hpp"

int main() {
  using namespace uhd_helper;

  if (geteuid() != 0) {
    std::cerr << "This tool must be run as root (sudo or root user).\n";
    return 1;
  }

  ConfigManager config_manager(DefaultConfigPath());
  ProfileManager profile_manager(&config_manager);

  std::string error;
  if (!profile_manager.Initialize(&error)) {
    std::cerr << "Failed to initialize: " << error << "\n";
    return 1;
  }

  TuiApp app(&profile_manager);
  app.Run();
  return 0;
}
