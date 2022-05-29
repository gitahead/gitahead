set(CONF_DIR ${CMAKE_SOURCE_DIR}/conf)
set(RSRC_DIR ${CMAKE_SOURCE_DIR}/rsrc)

set(MAC $<PLATFORM_ID:Darwin>)
set(CONTENTS_DIR ${GITAHEAD_NAME}.app/Contents)

# Install Qt plugins.
set(QT_PLUGINS
  QJpegPlugin
)

if(APPLE)
  set(QT_PLUGINS
    ${QT_PLUGINS}
    QCocoaIntegrationPlugin
    QMacStylePlugin
  )
elseif(WIN32)
  set(QT_PLUGINS
    ${QT_PLUGINS}
    QWindowsIntegrationPlugin
    QWindowsVistaStylePlugin
  )
else()
  set(QT_PLUGINS
    ${QT_PLUGINS}
    QXcbIntegrationPlugin
    QComposePlatformInputContextPlugin
  )
endif()

foreach(QT_PLUGIN ${QT_PLUGINS})
  if(NOT TARGET Qt5::${QT_PLUGIN})
    continue()
  endif()

  get_target_property(PLUGIN Qt5::${QT_PLUGIN} LOCATION_${CMAKE_BUILD_TYPE})
  get_filename_component(FILE_NAME ${PLUGIN} NAME)
  get_filename_component(DIR ${PLUGIN} DIRECTORY)
  get_filename_component(DIR_NAME ${DIR} NAME)
  set(PLUGIN_PATH Plugins/${DIR_NAME}/${FILE_NAME})

  install(FILES ${PLUGIN}
    DESTINATION $<${MAC}:${CONTENTS_DIR}/>Plugins/${DIR_NAME}
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
      GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${GITAHEAD_NAME}
  )

  if(WIN32)
    # Copy into build dir.
    execute_process(COMMAND
      ${CMAKE_COMMAND} -E copy_if_different
        ${PLUGIN} ${CMAKE_BINARY_DIR}/${PLUGIN_PATH}
    )
  elseif(APPLE)
    # Sign plugin.
    if(CODESIGN_IDENTITY)
      install(CODE
        "execute_process(COMMAND
          codesign --timestamp --options runtime
            -s \"${CODESIGN_IDENTITY}\"
            \${CMAKE_INSTALL_PREFIX}/${CONTENTS_DIR}/${PLUGIN_PATH}
        )"
        COMPONENT ${GITAHEAD_NAME}
      )
    endif()
  else()
    # Delete runpath.
    install(CODE
      "execute_process(COMMAND chrpath --delete
        \${CMAKE_INSTALL_PREFIX}/${PLUGIN_PATH}
      )"
      COMPONENT ${GITAHEAD_NAME}
    )
  endif()
endforeach()

# Install Qt libraries.
foreach(QT_MODULE ${QT_MODULES})
  get_target_property(QT_LIBRARY Qt5::${QT_MODULE} LOCATION_${CMAKE_BUILD_TYPE})

  if(APPLE)
    get_filename_component(QT_FRAMEWORK ${QT_LIBRARY} DIRECTORY)
    set(DEST_DIR ${CONTENTS_DIR}/Frameworks)
    install(DIRECTORY ${QT_FRAMEWORK}
      DESTINATION ${DEST_DIR}
      COMPONENT ${GITAHEAD_NAME}
      PATTERN "*.prl" EXCLUDE
      PATTERN "*_debug" EXCLUDE
      PATTERN "Headers" EXCLUDE
      PATTERN ".DS_Store" EXCLUDE
    )
  else()
    if(WIN32)
      get_filename_component(TARGET_NAME ${QT_LIBRARY} NAME)

      # Copy into build dir.
      execute_process(COMMAND
        ${CMAKE_COMMAND} -E copy_if_different
          ${QT_LIBRARY} ${CMAKE_BINARY_DIR}
      )
    else()
      # Strip minor and patch version extensions.
      get_filename_component(TARGET_NAME_WE ${QT_LIBRARY} NAME_WE)
      set(TARGET_NAME ${TARGET_NAME_WE}.so.5)
    endif()

    install(FILES ${QT_LIBRARY}
      DESTINATION .
      PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
      COMPONENT ${GITAHEAD_NAME}
      RENAME ${TARGET_NAME}
    )

    if(NOT WIN32)
      # Delete runpath.
      install(CODE
        "execute_process(COMMAND chrpath --delete
          \${CMAKE_INSTALL_PREFIX}/${TARGET_NAME}
        )"
        COMPONENT ${GITAHEAD_NAME}
      )
    endif()
  endif()
endforeach()

# Install XcbQpa library.
if(UNIX AND NOT APPLE)
  set(LIB_NAME libQt5XcbQpa)
  get_target_property(QT_CORE_LIBRARY Qt5::Core LOCATION)
  get_filename_component(LIB_PATH ${QT_CORE_LIBRARY} PATH)
  get_filename_component(LIB_EXT ${QT_CORE_LIBRARY} EXT)

  install(FILES "${LIB_PATH}/${LIB_NAME}${LIB_EXT}"
    DESTINATION .
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
      GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${GITAHEAD_NAME}
    RENAME ${LIB_NAME}.so.5
  )
endif()

# Install SSL libraries.
if(NOT APPLE)
  if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      set(SSL_LIB_SUFFIX "-1_1-x64.dll")
    else()
      set(SSL_LIB_SUFFIX "-1_1.dll")
    endif()
  else()
    set(SSL_LIB_SUFFIX ".so.1.1")
  endif()

  foreach(SSL_LIB_NAME ssl crypto)
    set(SSL_LIB "${OPENSSL_DIR}/lib${SSL_LIB_NAME}${SSL_LIB_SUFFIX}")

    if(WIN32)
      # Copy into build dir.
      execute_process(COMMAND
        ${CMAKE_COMMAND} -E copy_if_different
          ${SSL_LIB} ${CMAKE_BINARY_DIR}
      )
    endif()

    install(FILES ${SSL_LIB}
      DESTINATION .
      PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
      COMPONENT ${GITAHEAD_NAME}
      OPTIONAL # if we use the system libssh, we have not built openssl
    )
  endforeach()
endif()

# Install config files.
set(QT_CONF ${CONF_DIR}/qt.conf)
install(FILES ${QT_CONF}
  DESTINATION $<${MAC}:${CONTENTS_DIR}/Resources/>.
  COMPONENT ${GITAHEAD_NAME}
)

if(NOT APPLE)
  if(UNIX)
    install(DIRECTORY ${RSRC_DIR}/GitAhead.iconset
      DESTINATION Resources
      COMPONENT ${GITAHEAD_NAME}
    )
  endif()
endif()

# Sign bundle on Mac.
if(APPLE AND CODESIGN_IDENTITY)
  install(CODE
    "execute_process(COMMAND
      codesign --deep --timestamp --options runtime
        -s \"${CODESIGN_IDENTITY}\"
        \${CMAKE_INSTALL_PREFIX}/${GITAHEAD_NAME}.app
    )"
    COMPONENT ${GITAHEAD_NAME}
  )
endif()

# Add package rule.
if(APPLE)
  set(CPACK_GENERATOR DragNDrop)
  set(CPACK_DMG_VOLUME_NAME ${GITAHEAD_NAME})
  set(CPACK_DMG_DS_STORE ${RSRC_DIR}/DS_Store)
  set(CPACK_DMG_BACKGROUND_IMAGE ${RSRC_DIR}/background.png)
elseif(WIN32)
  set(CPACK_GENERATOR NSIS)
  set(CPACK_NSIS_CREATE_ICONS_EXTRA
    "CreateShortCut \
    \\\"$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\${GITAHEAD_NAME}.lnk\\\" \
    \\\"$INSTDIR\\\\${GITAHEAD_NAME}.exe\\\""
  )
  set (CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
  set (CPACK_NSIS_MUI_FINISHPAGE_RUN ${GITAHEAD_NAME}.exe)

  if(CMAKE_CL_64)
    install(FILES ${RSRC_DIR}/vcredist_x64.exe
      DESTINATION .
      COMPONENT ${GITAHEAD_NAME}
    )
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
      "ExecWait '\\\"$INSTDIR\\\\vcredist_x64.exe\\\" /q /norestart'"
    )
    set(PLATFORM "-win64")
  else()
    install(FILES ${RSRC_DIR}/vcredist_x86.exe
      DESTINATION .
      COMPONENT ${GITAHEAD_NAME}
    )
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
      "ExecWait '\\\"$INSTDIR\\\\vcredist_x86.exe\\\" /q /norestart'"
    )
    set(PLATFORM "-win32")
  endif()
else()
  set(CPACK_GENERATOR STGZ)
endif()

set(PACKAGE_FILE_NAME "${GITAHEAD_NAME}${PLATFORM}-${GITAHEAD_VERSION}")

set(CPACK_MONOLITHIC_INSTALL true)
set(CPACK_INSTALL_CMAKE_PROJECTS
  ${CMAKE_BINARY_DIR} ${GITAHEAD_NAME} ${GITAHEAD_NAME} /
)

# Add Context Menu Shortcuts
if(WIN32)
  set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}
    WriteRegStr HKCR 'Directory\\\\Background\\\\shell\\\\GitAhead' '' 'Open with GitAhead'
    WriteRegStr HKCR 'Directory\\\\Background\\\\shell\\\\GitAhead' 'Icon' '\\\"$INSTDIR\\\\gitahead.exe\\\"'
    WriteRegStr HKCR 'Directory\\\\Background\\\\shell\\\\GitAhead\\\\command' '' '\\\"$INSTDIR\\\\gitahead.exe\\\" \\\"%V\\\"'
    WriteRegStr HKCR 'Directory\\\\shell\\\\GitAhead' '' 'Open with GitAhead'
    WriteRegStr HKCR 'Directory\\\\shell\\\\GitAhead' 'Icon' '\\\"$INSTDIR\\\\gitahead.exe\\\"'
    WriteRegStr HKCR 'Directory\\\\shell\\\\GitAhead\\\\command' '' '\\\"$INSTDIR\\\\gitahead.exe\\\" \\\"%L\\\"'"
  )
  set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "${CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS}
    DeleteRegKey HKCR 'Directory\\\\Background\\\\shell\\\\GitAhead'
    DeleteRegKey HKCR 'Directory\\\\shell\\\\GitAhead'"
  )
endif(WIN32)

set(CPACK_PACKAGE_FILE_NAME ${PACKAGE_FILE_NAME})
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${GITAHEAD_NAME})
set(CPACK_PACKAGE_VENDOR "Scientific Toolworks, Inc.")
set(CPACK_PACKAGE_CONTACT "https://github.com/gitahead/gitahead")
set(CPACK_PACKAGE_VERSION_MAJOR ${GITAHEAD_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${GITAHEAD_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${GITAHEAD_VERSION_PATCH})
set(CPACK_DEBIAN_PACKAGE_NAME "gitahead")
set(CPACK_DEBIAN_PACKAGE_SECTION "universe/devel")
set(CPACK_DEBIAN_COMPRESSION_TYPE "xz")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS_POLICY ">=")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "git")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "git-lfs")
set(CPACK_DEBIAN_PACKAGE_ENHANCES "git,git-lfs")


if(WIN32)
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE.md)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
include(CPack)

if(WIN32)
  set(PACKAGE_DIR ${CMAKE_BINARY_DIR}/${PACKAGE_FILE_NAME})

  # Add target to package and sign executable.
  add_custom_target(deploy
    signtool sign /v /f ${CERTIFICATE_FILE} /p ${CERTIFICATE_PASSWORD}
      ${CMAKE_BINARY_DIR}/${PACKAGE_FILE_NAME}.exe
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PACKAGE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_BINARY_DIR}/GitAhead.exe ${PACKAGE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_BINARY_DIR}/GitAhead.pdb ${PACKAGE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_BINARY_DIR}/indexer.exe ${PACKAGE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_BINARY_DIR}/indexer.pdb ${PACKAGE_DIR}
  )

  add_dependencies(deploy package)
endif()
