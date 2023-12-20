cmake_minimum_required(VERSION 3.15)
project(GitAhead)

# Set name and version.
set(GITAHEAD_NAME "GitAhead")
set(GITAHEAD_VERSION_MAJOR 2)
set(GITAHEAD_VERSION_MINOR 7)
set(GITAHEAD_VERSION_PATCH 1)
set(GITAHEAD_VERSION
  "${GITAHEAD_VERSION_MAJOR}.${GITAHEAD_VERSION_MINOR}.${GITAHEAD_VERSION_PATCH}"
)

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

# Require C++17.
set(CMAKE_CXX_STANDARD 17)
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
  Core5Compat
  Gui
  Network
  PrintSupport
  Widgets
  Test
)

if(APPLE)
  set(QT_MODULES ${QT_MODULES} DBus)
endif()

find_package(Qt6 COMPONENTS ${QT_MODULES} LinguistTools REQUIRED)

set(QT_TRANSLATIONS_DIR "${Qt6_DIR}/../../../translations")

if(APPLE)
  foreach(QT_MODULE ${QT_MODULES})
    # FIXME: Force debug build to link against release libraries
    # inside of frameworks on macOS. The Qt debug libraries are
    # broken because their link dependencies are to the release
    # libraries (e.g. QtGui_debug depends on QtCore). This causes
    # multiple symbol definition errors at application load time.
    get_target_property(LOCATION Qt6::${QT_MODULE} LOCATION)
    set_target_properties(Qt6::${QT_MODULE} PROPERTIES
      IMPORTED_LOCATION_DEBUG ${LOCATION}
    )
  endforeach()
endif()

add_subdirectory(dep)
add_subdirectory(src)
add_subdirectory(l10n)
add_subdirectory(test)
add_subdirectory(pack)
