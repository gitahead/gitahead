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
  CONF_DIR="${CMAKE_INSTALL_FULL_DATADIR}/gitahead"
  SCINTILLUA_LEXERS_DIR="${CMAKE_INSTALL_FULL_DATADIR}/gitahead/lexers"
  HELPERS_DIR="${CMAKE_INSTALL_FULL_LIBEXECDIR}"
)

set_target_properties(conf PROPERTIES
  AUTOMOC ON
)
