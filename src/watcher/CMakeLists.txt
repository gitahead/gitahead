if(APPLE)
  set(WATCHER_IMPL_FILE RepositoryWatcher_mac.cpp)
  set(WATCHER_IMPL_LIBS "-framework CoreServices")
elseif(WIN32)
  set(WATCHER_IMPL_FILE RepositoryWatcher_win.cpp)
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
  # if we're in FreeBSD then we can use the linux inotify watcher
  # but... we need the libinotify bridge to kqueue, so we need to find this
  include(FindPkgConfig)
  pkg_check_modules(LIBINOTIFY REQUIRED libinotify)

  set(WATCHER_IMPL_LIBS ${LIBINOTIFY_LINK_LIBRARIES})
  set(WATCHER_IMPL_INC_DIRS ${LIBINOTIFY_INCLUDE_DIRS})
  set(WATCHER_IMPL_FILE RepositoryWatcher_linux.cpp)
else()
  set(WATCHER_IMPL_FILE RepositoryWatcher_linux.cpp)
endif()

add_library(watcher
  RepositoryWatcher.cpp
  ${WATCHER_IMPL_FILE}
)

target_include_directories(watcher PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${WATCHER_IMPL_INC_DIRS}
)

target_link_libraries(watcher
  git
  Qt5::Core
  ${WATCHER_IMPL_LIBS}
)

set_target_properties(watcher PROPERTIES
  AUTOMOC ON
)
