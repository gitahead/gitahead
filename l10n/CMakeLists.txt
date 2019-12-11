option(UPDATE_TRANSLATIONS
  "Update translation (*.ts) files when source files change"
)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
file(GLOB_RECURSE SOURCE_FILES
  ${SRC_DIR}/*.h
  ${SRC_DIR}/*.cpp
  ${SRC_DIR}/*.mm
)

file(GLOB TS_FILES *.ts)

if(UPDATE_TRANSLATIONS)
  # FIXME: Clean removes the .ts files.
  qt5_create_translation(QM_FILES ${SOURCE_FILES} ${TS_FILES})
else()
  qt5_add_translation(QM_FILES ${TS_FILES})
endif()

add_custom_target(translations DEPENDS ${QM_FILES})
add_dependencies(gitahead translations)

foreach(TS_FILE ${TS_FILES})
  if(TS_FILE MATCHES ^.*_\([a-zA-Z_-]+\)\\.ts$
     AND NOT CMAKE_MATCH_1 IN_LIST LANGUAGES)
    set(LANGUAGES ${LANGUAGES} ${CMAKE_MATCH_1})
  endif()
endforeach()

foreach(LANGUAGE ${LANGUAGES})
  set(QT_QM_FILES ${QT_QM_FILES}
    ${QT_TRANSLATIONS_DIR}/qt_${LANGUAGE}.qm
  )
endforeach()

set(MAC $<PLATFORM_ID:Darwin>)
set(CONTENTS_DIR ${GITAHEAD_NAME}.app/Contents)
foreach(QM_FILE ${QM_FILES} ${QT_QM_FILES})
  install(FILES ${QM_FILE}
    DESTINATION $<${MAC}:${CONTENTS_DIR}/>Resources/l10n
    COMPONENT ${GITAHEAD_NAME}
  )
endforeach()
