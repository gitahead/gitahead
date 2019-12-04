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
