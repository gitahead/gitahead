--
-- Many colors support 'active', 'inactive', and 'disabled' states.
-- They can all be set to the same color with the syntax:
--
--   key = '<color>'
--
-- Or set individually with syntax like:
--
--   key = { active = '<color>', inactive = '<color>', disabled = '<color>' }
--
-- Use the 'default' key set one state individually and the remainder
-- to a default value:
--
--   key = { default = '<color>', disabled = '<color>' }
--

-- generic colors used to render borders and separators
-- { default, active, inactive, disabled }
theme['palette']   = {
  -- These names correspond to a dark on light theme.
  -- The values should be inverted in light on dark themes.
  light            = '#E3E3E8', -- inverse of dark
  midlight         = '#EAEAEB', -- inverse of middark
  middark          = '#D0D0D2', -- inverse of midlight
  dark             = '#C3C3C5', -- inverse of light

  -- This should always be a dark color.
  shadow           = '#A1A1A4'
}

-- the colors of text entry, list view, and other widgets
-- { default, active, inactive, disabled }
theme['widget']    = {
  text             = { default = '#FFFFFF', disabled = '#555B65' },
  bright_text      = '#C3C3C5',
  background       = '#36373E',
  highlight        = { active = '#2A82DA', inactive = '#1B5B9B' },
  highlighted_text = { active = '#E1E5F2', inactive = '#E1E5F2' },
}

-- window colors
-- { default, active, inactive, disabled }
theme['window']    = {
  text             = '#4A4F51',
  background       = '#DDDDDE',
  bright_text      = '#767E82'
}

-- button colors
-- { default, active, inactive, disabled, checked, pressed }
theme['button']    = {
  background       = { default = '#DDDDDE', checked = '#2A82DA', pressed = '#a9a9ac' },
  text             = { default = '#4A4F51', disabled = '#555B65', checked = '#FFFFFF' }
}

-- commit list colors
-- { default, active, inactive, disabled }
theme['commits']   = {
  background       = '#DDDDDE',
  text             = '#4A4F51',
  bright_text      = '#767E82',
  highlight        = { active = '#2A82DA', inactive = '#136dc6' },
  highlighted_text = { active = '#E1E5F2', inactive = '#E1E5F2' },
  highlighted_bright_text = { active = '#A6CBF0', inactive = '#C3C3C5' }
}

-- file list colors
-- { default, active, inactive, disabled }
theme['files']     = {
  background       = '#E3E3E8',
  alternate        = '#D0D0D2',
  text             = '#4A4F51',
  highlight        = { active = '#2A82DA', inactive = '#136dc6' },
  highlighted_text = { active = '#E1E5F2', inactive = '#E1E5F2' }
}

-- status badge colors
-- { normal, selected, conflicted, head, notification }
theme['badge']     = {
  background       = {
    normal         = '#2A82DA', -- the default color
    selected       = '#E1E5F2', -- the color when a list item is selected
    conflicted     = '#DA2ADA', -- the color of conflicted items
    head           = '#005DBA', -- a bolder color to indicate the HEAD
    notification   = '#8C2026'  -- the color of toolbar notifications badges
  },
  foreground       = {
    normal         = '#E1E5F2',
    selected       = '#2A82DA'
  }
}

-- blame margin heatmap background colors
theme['blame'] = {
  cold             = '#282940',
  hot              = '#5E3638'
}

-- graph edge colors
theme['graph']     = {
  edge1            = '#53AFEC',
  edge2            = '#82DA2A',
  edge3            = '#DA2ADA',
  edge4            = '#DA822A',
  edge5            = '#2ADADA',
  edge6            = '#DA2A82',
  edge7            = '#84A896',
  edge8            = '#2ADA82',
  edge9            = '#822ADA',
  edge10           = '#66D1E0',
  edge11           = '#D3C27E',
  edge12           = '#95CB80',
  edge13           = '#50D4BE',
  edge14           = '#2ADA82',
  edge15           = '#DA822A'
}

-- checkbox colors
-- { default, active, inactive, disabled }
theme['checkbox']  = {
  fill             = '#535359',
  outline          = '#3C3C42',
  text             = '#FFFFFF'
}

-- diff view colors
theme['diff']      = {
  addition         = '#394734', -- added lines
  deletion         = '#5E3638', -- deleted lines
  plus             = '#207A00', -- plus icon
  minus            = '#BC0009', -- minus icon
  ours             = '#000060', -- ours conflict lines
  theirs           = '#600060', -- theirs conflict lines
  word_addition    = '#296812', -- added words
  word_deletion    = '#781B20', -- deleted words
  note             = '#E1E5F2', -- note squiggle
  warning          = '#E8C080', -- warning background
  error            = '#7E494B'  -- error background
}

-- list view footer colors
-- { default, active, inactive, disabled }
theme['footer']    = {
  button           = { active = '#4A4F51', disabled = '#728094' },
}

-- link colors
-- { default, active, inactive, disabled }
theme['link']      = {
  link             = '#2A82DA',
  link_visited     = '#FF00FF'
}

-- menubar background color
theme['menubar']   = {
  background       = '#F0F0F0',
  text             = '#212226'
}

-- star fill color
-- { default, active, inactive, disabled }
theme['star']      = {
  fill             = '#FFFFFF'
}

-- titlebar background color (currently only supported on macOS)
theme['titlebar']  = {
  background = '#D0D0D1'
}

-- popup tooltip colors
-- { default, active, inactive, disabled }
theme['tooltip']   = {
  background       = '#2A82DA',
  text             = '#E1E5F2'
}

-- editor styles
-- Styles are composed of a string like:
--   fore:<color>,back:<color>,bold,italics,underline
-- Symbolic style names are allowed:
--   $(style.name)
-- http://www.scintilla.org/MyScintillaDoc.html#Styling
theme.property['style.bracebad']     = 'fore:#CC8080'
theme.property['style.bracelight']   = 'fore:#80CCFF'
theme.property['style.calltip']      = 'fore:#AAB2BE,back:#333333'
theme.property['style.class']        = 'fore:#F6E9D0'
theme.property['style.comment']      = 'fore:#E2D9C9'
theme.property['style.constant']     = 'fore:#E8C080'
theme.property['style.controlchar']  = '$(style.nothing)'
theme.property['style.default']      = 'fore:#AAB2BE,back:#36373E'
theme.property['style.definition']   = 'fore:#F6E9D0'
theme.property['style.embedded']     = '$(style.tag),back:#333333'
theme.property['style.error']        = 'fore:#994D4D,italics'
theme.property['style.function']     = 'fore:#4D99E6'
theme.property['style.identifier']   = '$(style.nothing)'
theme.property['style.indentguide']  = 'fore:#333333,back:#333333'
theme.property['style.keyword']      = 'fore:#53AFEC,bold'
theme.property['style.label']        = 'fore:#E8C080'
theme.property['style.linenumber']   = 'fore:#5F6672,back:#2A2B30,bold'
theme.property['style.nothing']      = ''
theme.property['style.number']       = 'fore:#4D99E6'
theme.property['style.operator']     = 'fore:#CCCCCC,bold'
theme.property['style.preprocessor'] = 'fore:#CC77DA,bold'
theme.property['style.regex']        = 'fore:#80CC80'
theme.property['style.string']       = 'fore:#93C37E'
theme.property['style.tag']          = 'fore:#CCCCCC'
theme.property['style.type']         = 'fore:#CC77DA'
theme.property['style.variable']     = 'fore:#80CCFF'
theme.property['style.whitespace']   = ''
