set(CONF_DIR ${CMAKE_SOURCE_DIR}/conf)
set(RSRC_DIR ${CMAKE_SOURCE_DIR}/rsrc)

if(APPLE)
  set(ICON ${RSRC_DIR}/GitAhead.icns)
  set(APP_IMPL_FILE Application_mac.mm)
  set(THEME_IMPL_FILE CustomTheme_mac.mm)
elseif(WIN32)
  set(RC_FILE ${RSRC_DIR}/GitAhead.rc)
endif()

set(EMOJI ${CONF_DIR}/emoji.json)
set(MERGETOOLS ${CONF_DIR}/mergetools)

file(GLOB CONFIG_FILES ${CONF_DIR}/*.lua)
file(GLOB DICTIONARIES ${CONF_DIR}/dictionaries/*.aff ${CONF_DIR}/dictionaries/*.dic ${CONF_DIR}/dictionaries/*.txt)
file(GLOB THEMES ${CONF_DIR}/themes/*.lua)
file(GLOB SCINTILLUA_LEXERS ${SCINTILLUA_LEXERS_DIR}/*.lua)
file(GLOB LUA_PLUGINS ${CONF_DIR}/plugins/*.lua ${CONF_DIR}/plugins/*.txt)

# Build resources.
qt_add_resources(RESOURCES ${RSRC_DIR}/resources.qrc)

# Generate acknowledgments.
set(DOC_SOURCE_DIR ${CMAKE_SOURCE_DIR}/doc)
set(DOC_BINARY_DIR ${CMAKE_BINARY_DIR}/doc)
set(CHANGELOG_MD ${DOC_SOURCE_DIR}/changelog.md)
set(ACKNOWLEDGMENTS_MD ${DOC_SOURCE_DIR}/acknowledgments.md)
set(PLUGIN_MD ${DOC_SOURCE_DIR}/plugin.md)
set(CHANGELOG_HTML ${DOC_BINARY_DIR}/changelog.html)
set(ACKNOWLEDGMENTS_HTML ${DOC_BINARY_DIR}/acknowledgments.html)
set(PLUGIN_HTML ${DOC_BINARY_DIR}/plugin.html)
add_custom_command(OUTPUT
  ${CHANGELOG_HTML} ${ACKNOWLEDGMENTS_HTML} ${PLUGIN_HTML}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${DOC_BINARY_DIR}
  COMMAND cmark ${CHANGELOG_MD} > ${CHANGELOG_HTML}
  COMMAND cmark ${ACKNOWLEDGMENTS_MD} > ${ACKNOWLEDGMENTS_HTML}
  COMMAND cmark ${PLUGIN_MD} > ${PLUGIN_HTML}
  DEPENDS ${CHANGELOG_MD} ${ACKNOWLEDGMENTS_MD} ${PLUGIN_MD}
)

# Add application library.
add_library(app
  Application.cpp
  CustomTheme.cpp
  Theme.cpp
  ${RESOURCES}
  ${APP_IMPL_FILE}
  ${THEME_IMPL_FILE}
)

target_compile_definitions(app
  PUBLIC
    GITAHEAD_NAME="${GITAHEAD_NAME}"
    GITAHEAD_VERSION="${GITAHEAD_VERSION}"
  PRIVATE
    QT_TRANSLATIONS_DIR="${QT_TRANSLATIONS_DIR}"
)

target_link_libraries(app
  conf
  git
  ui
  update
  Qt6::Widgets
)

set_target_properties(app PROPERTIES
  AUTOMOC ON
)

# Add main executable.
add_executable(gitahead WIN32 MACOSX_BUNDLE
  GitAhead.cpp
  ${ICON}
  ${RC_FILE}
  ${CONFIG_FILES}
  ${DICTIONARIES}
  ${EMOJI}
  ${MERGETOOLS}
  ${THEMES}
  ${LUA_PLUGINS}
  ${SCINTILLUA_LEXERS}
  ${CHANGELOG_HTML}
  ${ACKNOWLEDGMENTS_HTML}
  ${PLUGIN_HTML}
)

target_link_libraries(gitahead app)

set_target_properties(gitahead PROPERTIES
  OUTPUT_NAME ${GITAHEAD_NAME}
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

if(WIN32)
  target_link_libraries(app Dbghelp.lib)
elseif(APPLE)
  set_target_properties(gitahead PROPERTIES
    INSTALL_RPATH "@loader_path/../Frameworks"
    MACOSX_BUNDLE_INFO_PLIST ${RSRC_DIR}/Info.plist.in
    MACOSX_BUNDLE_ICON_FILE ${GITAHEAD_NAME}
    MACOSX_BUNDLE_BUNDLE_NAME ${GITAHEAD_NAME}
    MACOSX_BUNDLE_BUNDLE_VERSION ${GITAHEAD_VERSION}
    MACOSX_BUNDLE_LONG_VERSION_STRING ${GITAHEAD_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${GITAHEAD_VERSION}
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.gitahead.GitAhead"
  )

  # Copy config files into bundle.
  set_source_files_properties(
    ${ICON} ${EMOJI} ${MERGETOOLS} ${CONFIG_FILES}
    ${CHANGELOG_HTML} ${ACKNOWLEDGMENTS_HTML} ${PLUGIN_HTML}
    PROPERTIES MACOSX_PACKAGE_LOCATION Resources
  )

  set_source_files_properties(${DICTIONARIES} PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources/dictionaries
  )

  set_source_files_properties(${THEMES} PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources/themes
  )

  set_source_files_properties(${LUA_PLUGINS} PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources/plugins
  )

  set_source_files_properties(${SCINTILLUA_LEXERS} PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources/lexers
  )
else()
  set_target_properties(gitahead PROPERTIES
    INSTALL_RPATH "$ORIGIN"
  )
endif()

add_dependencies(gitahead indexer relauncher)

# Install application.
install(TARGETS gitahead
  DESTINATION .
  COMPONENT ${GITAHEAD_NAME}
)

if(APPLE)
  get_target_property(QT_LIBRARY Qt6::Core LOCATION)
  get_filename_component(QT_FRAMEWORK ${QT_LIBRARY} DIRECTORY)
  get_filename_component(RPATH ${QT_FRAMEWORK} DIRECTORY)
  get_filename_component(RPATH ${RPATH} DIRECTORY)
  get_filename_component(RPATH ${RPATH} DIRECTORY)

  # Fixup relauncher RPATH.
  set(RELAUNCHER ${GITAHEAD_NAME}.app/Contents/MacOS/relauncher)
  install(CODE
    "execute_process(COMMAND
      ${CMAKE_INSTALL_NAME_TOOL} -delete_rpath \"${RPATH}\"
        \"\${CMAKE_INSTALL_PREFIX}/${RELAUNCHER}\"
    )
    execute_process(COMMAND
      ${CMAKE_INSTALL_NAME_TOOL} -add_rpath \"@loader_path/../Frameworks\"
        \"\${CMAKE_INSTALL_PREFIX}/${RELAUNCHER}\"
    )"
    COMPONENT ${GITAHEAD_NAME}
  )

  # Fixup indexer RPATH.
  set(INDEXER ${GITAHEAD_NAME}.app/Contents/MacOS/indexer)
  install(CODE
    "execute_process(COMMAND
      ${CMAKE_INSTALL_NAME_TOOL} -delete_rpath \"${RPATH}\"
        \"\${CMAKE_INSTALL_PREFIX}/${INDEXER}\"
    )
    execute_process(COMMAND
      ${CMAKE_INSTALL_NAME_TOOL} -add_rpath \"@loader_path/../Frameworks\"
        \"\${CMAKE_INSTALL_PREFIX}/${INDEXER}\"
    )"
    COMPONENT ${GITAHEAD_NAME}
  )
else()
  # Install config files.
  if(CHANGELOG_HTML AND ACKNOWLEDGMENTS_HTML AND PLUGIN_HTML)
    install(FILES ${CHANGELOG_HTML} ${ACKNOWLEDGMENTS_HTML} ${PLUGIN_HTML}
      DESTINATION Resources
      COMPONENT ${GITAHEAD_NAME}
    )
  endif()

  install(FILES ${EMOJI} ${MERGETOOLS}
    DESTINATION Resources
    COMPONENT ${GITAHEAD_NAME}
  )

  foreach(CONFIG_FILE ${CONFIG_FILES})
    install(FILES ${CONFIG_FILE}
      DESTINATION Resources
      COMPONENT ${GITAHEAD_NAME}
    )
  endforeach()

  install(FILES ${DICTIONARIES}
    DESTINATION Resources/dictionaries
    COMPONENT ${GITAHEAD_NAME}
  )

  install(FILES ${THEMES}
    DESTINATION Resources/themes
    COMPONENT ${GITAHEAD_NAME}
  )

  install(FILES ${LUA_PLUGINS}
    DESTINATION Resources/plugins
    COMPONENT ${GITAHEAD_NAME}
  )

  foreach(SCINTILLUA_LEXER ${SCINTILLUA_LEXERS})
    install(FILES ${SCINTILLUA_LEXER}
      DESTINATION Resources/lexers
      COMPONENT ${GITAHEAD_NAME}
    )
  endforeach()
endif()
