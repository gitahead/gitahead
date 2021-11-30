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
  SCINTILLUA_LEXERS_DIR="${SCINTILLUA_LEXERS_DIR}"
)

if (FLATPAK)
    target_compile_definitions(conf PRIVATE
	CONF_DIR="/app/Resources"
	L10N_DIR="/app/Resources/l10n"
    )
else()
    target_compile_definitions(conf PRIVATE
	CONF_DIR="${CMAKE_SOURCE_DIR}/conf"
	L10N_DIR="${CMAKE_BINARY_DIR}/l10n"
    )
endif()

set_target_properties(conf PROPERTIES
  AUTOMOC ON
)
