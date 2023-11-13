option(UPDATE_TRANSLATIONS
  "Update translation (*.ts) files when source files change"
)

set(LANGUAGES
  de
  ru
  en
  es
  ja
  pt
  pt_BR
  zh_CN
)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
file(GLOB_RECURSE SOURCE_FILES
  ${SRC_DIR}/*.h
  ${SRC_DIR}/*.cpp
  ${SRC_DIR}/*.mm
)

foreach(LANGUAGE ${LANGUAGES})
  set(TS_FILES ${TS_FILES} gitahead_${LANGUAGE}.ts)
endforeach()

if(UPDATE_TRANSLATIONS)
  # FIXME: Clean removes the .ts files.
  qt_create_translation(QM_FILES ${SOURCE_FILES} ${TS_FILES})
else()
  qt_add_translation(QM_FILES ${TS_FILES})
endif()

add_custom_target(translations DEPENDS ${QM_FILES})
add_dependencies(gitahead translations)

set(CONTENTS_DIR ${GITAHEAD_NAME}.app/Contents)
foreach(LANGUAGE ${LANGUAGES})
  set(QT_QM_FILES ${QT_QM_FILES}
    ${QT_TRANSLATIONS_DIR}/qtbase_${LANGUAGE}.qm
  )

  if(APPLE)
    get_target_property(DIR gitahead RUNTIME_OUTPUT_DIRECTORY)
    add_custom_command(TARGET translations POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory
        ${DIR}/${CONTENTS_DIR}/Resources/${LANGUAGE}.lproj
    )
  endif()
endforeach()

set(MAC $<PLATFORM_ID:Darwin>)
foreach(QM_FILE ${QM_FILES} ${QT_QM_FILES})
  if(EXISTS ${QM_FILE})
    install(FILES ${QM_FILE}
      DESTINATION $<${MAC}:${CONTENTS_DIR}/>Resources/l10n
      COMPONENT ${GITAHEAD_NAME}
    )
  endif()
endforeach()
