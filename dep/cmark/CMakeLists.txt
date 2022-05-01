if(PKG_CONFIG_FOUND)
  pkg_check_modules(LIBCMARK libcmark)
endif()
if(NOT LIBCMARK_FOUND)
  set(CMARK_TESTS
      OFF
      CACHE BOOL "" FORCE)
  set(CMARK_SHARED
      OFF
      CACHE BOOL "" FORCE)

  add_subdirectory(cmark)
  target_include_directories(
    cmark_static
    INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cmark/src>
              $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/cmark/src>)
  set(LIBCMARK_LIBRARIES
      cmark_static
      CACHE INTERNAL "libcmark implementation" FORCE)
endif()
