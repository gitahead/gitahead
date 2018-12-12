add_library(conf
  ConfFile.cpp
  Settings.cpp
  RecentRepositories.cpp
  RecentRepository.cpp
)

target_link_libraries(conf
  lua
  Qt5::Core
)

target_compile_definitions(conf PRIVATE
  CONF_DIR="${CMAKE_SOURCE_DIR}/conf"
  SCINTILLUA_LEXERS_DIR="${SCINTILLUA_LEXERS_DIR}"
)

set_target_properties(conf PROPERTIES
  AUTOMOC ON
)
