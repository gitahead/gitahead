add_library(editor
  LexLPeg.cpp
  PlatQt.cpp
  ScintillaIFace.cpp
  ScintillaQt.cpp
  TextEditor.cpp
)

target_link_libraries(editor
  lpeg
  lua
  scintilla
  Qt6::Widgets
)

set_target_properties(editor PROPERTIES
  AUTOMOC ON
)
