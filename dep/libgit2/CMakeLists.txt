#pkg_check_modules(libgit2 libgit2>1.1.0) # Not now, currently it depends on not upstreamed features + some changes that cannot be upstreamed at all.

if(libgit2_FOUND)
  add_library(git2 INTERFACE)
  target_include_directories(git2 INTERFACE ${libgit2_INCLUDE_DIR})
  target_link_libraries(git2 INTERFACE ${libgit2_LIBRARIES})
else()
  # Set some defaults.
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  set(BUILD_CLAR OFF CACHE BOOL "" FORCE)
  set(STATIC_CRT OFF CACHE BOOL "" FORCE)

if(WIN32)
  # Set variables manually.
  set(OPENSSL_FOUND TRUE)
  set(OPENSSL_INCLUDE_DIR ${OPENSSL_DIR}/inc32)
  set(OPENSSL_LIBRARIES
    ${OPENSSL_DIR}/out32dll/libeay32.lib
    ${OPENSSL_DIR}/out32dll/ssleay32.lib
    )

    # Disable nanosecond precision.
    set(USE_NSEC OFF CACHE BOOL "" FORCE)
  endif()

  add_subdirectory(libgit2)
  target_include_directories(git2 INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/libgit2/include
  )
endif()
