# Set some defaults.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_CLAR OFF CACHE BOOL "" FORCE)
set(STATIC_CRT OFF CACHE BOOL "" FORCE)

# Exec system OpenSSH for SSH.
set(USE_SSH "exec" CACHE BOOL "" FORCE)

if(WIN32)
  # Disable nanosecond precision.
  set(USE_NSEC OFF CACHE BOOL "" FORCE)
endif()

add_subdirectory(libgit2)
target_include_directories(libgit2package INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/libgit2/include
)
