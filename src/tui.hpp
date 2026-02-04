#pragma once

#include <string>
#include <vector>

namespace uhd_helper {

class ProfileManager;

class TuiApp {
 public:
  explicit TuiApp(ProfileManager* manager);

  void Run();

 private:
  void ReloadProfiles();
  void SetStatus(const std::string& message, bool is_error);

  ProfileManager* manager_;
  std::vector<std::string> profile_labels_;
  std::vector<std::string> profile_ids_;
  int selected_index_ = 0;
  int last_selected_index_ = -1;
  bool profile_confirmed_ = false;
  std::string status_message_;
  bool status_is_error_ = false;
};

}  // namespace uhd_helper
