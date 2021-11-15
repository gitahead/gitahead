add_library(tools
  DiffTool.cpp
  EditTool.cpp
  ExternalTool.cpp
  MergeTool.cpp
  ShowTool.cpp
)

if (FLATPAK)
    target_compile_definitions(tools PRIVATE FLATPAK)
endif()

target_link_libraries(tools
  conf
  git
  Qt5::Gui
)

set_target_properties(tools PROPERTIES
  AUTOMOC ON
)
