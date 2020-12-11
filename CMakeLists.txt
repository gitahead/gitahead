cmake_minimum_required(VERSION 3.6.2)
project(GitAhead)

# Set name and version.
set(GITAHEAD_NAME "GitAhead")
set(GITAHEAD_VERSION_MAJOR 2)
set(GITAHEAD_VERSION_MINOR 6)
set(GITAHEAD_VERSION_PATCH 3)
set(GITAHEAD_VERSION
  "${GITAHEAD_VERSION_MAJOR}.${GITAHEAD_VERSION_MINOR}.${GITAHEAD_VERSION_PATCH}"
)

include(GNUInstallDirs)

# Generate build date.
string(TIMESTAMP GITAHEAD_BUILD_DATE)

# Lookup git revision.
find_package(Git)
if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} --git-dir=${CMAKE_SOURCE_DIR}/.git show -s --format=%h HEAD
    OUTPUT_VARIABLE GITAHEAD_BUILD_REVISION
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
else()
  set(GITAHEAD_BUILD_REVISION "unknown")
endif()

# Explicitly disable shared libraries.
set(BUILD_SHARED_LIBS OFF)

# Require C++11.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Disable exceptions and RTTI.
if(WIN32)
  string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
  string(REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")
endif()

if(UNIX AND NOT APPLE)
  set(CMAKE_EXE_LINKER_FLAGS -ldl)
endif()

# Find Qt modules.
set(QT_MODULES
  Concurrent
  Core
  Gui
  Network
  PrintSupport
  Widgets
  Test
)

if(APPLE)
  set(QT_MODULES ${QT_MODULES} DBus)
endif()

find_package(Qt5 5.9 COMPONENTS ${QT_MODULES} LinguistTools REQUIRED)

set(QT_TRANSLATIONS_DIR "${Qt5_DIR}/../../../translations")

if(APPLE)
  foreach(QT_MODULE ${QT_MODULES})
    # FIXME: Force debug build to link against release libraries
    # inside of frameworks on macOS. The Qt debug libraries are
    # broken because their link dependencies are to the release
    # libraries (e.g. QtGui_debug depends on QtCore). This causes
    # multiple symbol definition errors at application load time.
    get_target_property(LOCATION Qt5::${QT_MODULE} LOCATION)
    set_target_properties(Qt5::${QT_MODULE} PROPERTIES
      IMPORTED_LOCATION_DEBUG ${LOCATION}
    )
  endforeach()
endif()

add_subdirectory(dep)
add_subdirectory(src)
add_subdirectory(l10n)
add_subdirectory(test)
add_subdirectory(pack)
