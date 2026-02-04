cmake_minimum_required(VERSION 3.16)

project(uhd_helper LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(external/ftxui)

add_executable(main
  src/main.cpp
  src/tui.cpp
  src/config_util.cpp
  src/profile_util.cpp
  src/file_util.cpp
  src/res.cpp
)
target_link_libraries(main
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component
)
