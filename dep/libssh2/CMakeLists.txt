set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

add_compile_options(-Wno-implicit-function-declaration)

add_subdirectory(libssh2)

set(LIBSSH2_FOUND TRUE PARENT_SCOPE)
set(LIBSSH2_LIBRARIES libssh2_static PARENT_SCOPE)
set(LIBSSH2_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/libssh2/include PARENT_SCOPE)
