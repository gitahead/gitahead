add_library(conf
  ConfFile.cpp
  Settings.cpp
  RecentRepositories.cpp
  RecentRepository.cpp
)

target_link_libraries(conf
  lua
  Qt6::Core
)

target_compile_definitions(conf PRIVATE
  CONF_DIR="${CMAKE_SOURCE_DIR}/conf"
  L10N_DIR="${CMAKE_BINARY_DIR}/l10n"
  SCINTILLUA_LEXERS_DIR="${SCINTILLUA_LEXERS_DIR}"
)

set_target_properties(conf PROPERTIES
  AUTOMOC ON
)
