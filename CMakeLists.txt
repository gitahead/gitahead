cmake_minimum_required(VERSION 3.3.1)
project(GitAhead)

# Set name and version.
set(GITAHEAD_NAME "GitAhead")
set(GITAHEAD_VERSION_MAJOR 2)
set(GITAHEAD_VERSION_MINOR 4)
set(GITAHEAD_VERSION_PATCH 7)
set(GITAHEAD_VERSION
  "${GITAHEAD_VERSION_MAJOR}.${GITAHEAD_VERSION_MINOR}.${GITAHEAD_VERSION_PATCH}"
)

# Generate build date.
string(TIMESTAMP GITAHEAD_BUILD_DATE)

# Lookup git revision.
find_package(Git)
if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} show -s --format=%h HEAD
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

set(Qt5_DIR ${CMAKE_BINARY_DIR}/../qt/qtbase/lib/cmake/Qt5 CACHE PATH "")
find_package(Qt5 COMPONENTS ${QT_MODULES} REQUIRED)

add_subdirectory(dep)
add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(pack)
