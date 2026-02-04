#include "tui.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "config_util.hpp"
#include "profile_util.hpp"

namespace uhd_helper {
namespace {

std::string ProfileStatusLabel(const Profile& profile,
                               const std::string& active_id) {
  std::string label = profile.display_name;
  if (profile.id == active_id) {
    label += " [active]";
  }
  if (profile.is_official) {
    label += " (official)";
  }
  return label;
}

}  // namespace

TuiApp::TuiApp(ProfileManager* manager) : manager_(manager) {}

void TuiApp::SetStatus(const std::string& message, bool is_error) {
  status_message_ = message;
  status_is_error_ = is_error;
}

void TuiApp::ReloadProfiles() {
  profile_labels_.clear();
  profile_ids_.clear();
  const auto& profiles = manager_->Profiles();
  const std::string active_id = manager_->ActiveProfileId();
  for (const auto& profile : profiles) {
    profile_labels_.push_back(ProfileStatusLabel(profile, active_id));
    profile_ids_.push_back(profile.id);
  }
  if (selected_index_ >= static_cast<int>(profile_labels_.size())) {
    selected_index_ = std::max(0, static_cast<int>(profile_labels_.size()) - 1);
  }
  last_selected_index_ = selected_index_;
  profile_confirmed_ = !profile_ids_.empty();
  if (!profile_confirmed_) {
    SetStatus("No profiles found", true);
  } else {
    profile_confirmed_ = false;
  }
}

void TuiApp::Run() {
  using namespace ftxui;

  ReloadProfiles();

  std::string add_profile_name;
  bool show_add_modal = false;

  ScreenInteractive screen = ScreenInteractive::FitComponent();

  auto menu = Menu(&profile_labels_, &selected_index_);

  std::vector<std::string> action_labels = {"Apply", "Delete"};
  int action_index = 0;
  auto action_menu = Menu(&action_labels, &action_index);

  auto add_button = Button("Add", [&] {
    add_profile_name.clear();
    show_add_modal = true;
  });
  auto reset_button = Button("Reset Official", [&] {
    std::string error;
    if (manager_->ResetToOfficial(&error)) {
      ReloadProfiles();
      SetStatus("Official profile applied", false);
    } else {
      SetStatus(error, true);
    }
  });
  auto refresh_button = Button("Refresh", [&] {
    std::string error;
    if (manager_->RefreshFromDisk(&error)) {
      ReloadProfiles();
      SetStatus("Profiles refreshed", false);
    } else {
      SetStatus(error, true);
    }
  });
  auto quit_button = Button("Quit", [&] { screen.ExitLoopClosure()(); });

  auto bottom_buttons =
      Container::Horizontal({add_button, reset_button, refresh_button, quit_button});

  auto main_container =
      Container::Vertical({Container::Horizontal({menu, action_menu}),
                           bottom_buttons});

  auto add_input = Input(&add_profile_name, "profile name");
  auto add_confirm = Button("Create", [&] {
    std::string error;
    if (manager_->AddProfileFromActive(add_profile_name, &error)) {
      ReloadProfiles();
      SetStatus("Profile created", false);
      show_add_modal = false;
    } else {
      SetStatus(error, true);
    }
  });
  auto add_cancel = Button("Cancel", [&] { show_add_modal = false; });
  auto add_modal_container =
      Container::Vertical({add_input, add_confirm, add_cancel});

  auto main_renderer = Renderer(main_container, [&] {
    if (selected_index_ != last_selected_index_) {
      profile_confirmed_ = false;
      last_selected_index_ = selected_index_;
    }

    Element status = text(status_message_);
    if (status_is_error_) {
      status = status | color(Color::RedLight);
    } else {
      status = status | color(Color::GreenLight);
    }

    Element menu_box =
        vbox({text("Profiles"), separator(), menu->Render()}) | border;

    Element action_box =
        vbox({text("Actions"), separator(), action_menu->Render()}) | border;
    if (!profile_confirmed_) {
      action_box = action_box | dim;
    }

    Element hint = text("Config: " + manager_->ConfigPath().string()) | dim;

    Element content = vbox({
        hbox({menu_box | flex, action_box | size(WIDTH, EQUAL, 24)}),
        hbox({add_button->Render(), reset_button->Render(),
              refresh_button->Render(), quit_button->Render()}) |
            border,
        status | border,
        hint,
    });
    return content;
  });

  auto modal_renderer = Renderer(add_modal_container, [&] {
    return window(
        text("Add Profile"),
        vbox({
            text("Create a profile from current images"),
            separator(),
            add_input->Render(),
            hbox({add_confirm->Render(), add_cancel->Render()}),
        })) |
           center;
  });

  auto root = Modal(main_renderer, modal_renderer, &show_add_modal);

  root = CatchEvent(root, [&](Event event) {
    if (show_add_modal && event == Event::Escape) {
      show_add_modal = false;
      return true;
    }
    if (!show_add_modal && menu->Focused() && event == Event::Return) {
      if (profile_ids_.empty()) {
        SetStatus("No profiles available", true);
      } else {
        profile_confirmed_ = true;
        SetStatus("Profile selected. Choose an action.", false);
      }
      return true;
    }
    if (event == Event::Return && !show_add_modal) {
      if (!profile_confirmed_) {
        SetStatus("Select a profile first (Enter)", true);
        return true;
      }
      if (action_index == 0) {
        if (profile_ids_.empty()) {
          SetStatus("No profiles available", true);
          return true;
        }
        std::string error;
        if (manager_->ApplyProfile(profile_ids_[selected_index_], &error)) {
          ReloadProfiles();
          SetStatus("Profile applied", false);
        } else {
          SetStatus(error, true);
        }
        return true;
      }
      if (action_index == 1) {
        if (profile_ids_.empty()) {
          SetStatus("No profiles available", true);
          return true;
        }
        std::string error;
        if (manager_->DeleteProfile(profile_ids_[selected_index_], &error)) {
          ReloadProfiles();
          SetStatus("Profile deleted", false);
        } else {
          SetStatus(error, true);
        }
        return true;
      }
    }
    return false;
  });

  screen.Loop(root);
}

}  // namespace uhd_helper
