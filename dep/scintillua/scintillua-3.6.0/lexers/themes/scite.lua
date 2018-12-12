-- Copyright 2006-2015 Mitchell mitchell.att.foicica.com. See LICENSE.
-- SciTE lexer theme for Scintillua.

local property = lexer.property

property['color.red'] = '#7F0000'
property['color.yellow'] = '#7F7F00'
property['color.green'] = '#007F00'
property['color.teal'] = '#007F7F'
property['color.purple'] = '#7F007F'
property['color.orange'] = '#B07F00'
property['color.blue'] = '#00007F'
property['color.black'] = '#000000'
property['color.grey'] = '#808080'
property['color.white'] = '#FFFFFF'

-- Default style.
local font, size = 'Monospace', 11
if WIN32 then
  font = 'Courier New'
elseif OSX then
  font, size = 'Monaco', 12
end
property['style.default'] = 'font:'..font..',size:'..size..
                            ',fore:$(color.black),back:$(color.white)'

-- Token styles.
property['style.nothing'] = ''
property['style.class'] = 'fore:$(color.black),bold'
property['style.comment'] = 'fore:$(color.green)'
property['style.constant'] = 'fore:$(color.teal),bold'
property['style.definition'] = 'fore:$(color.black),bold'
property['style.error'] = 'fore:$(color.red)'
property['style.function'] = 'fore:$(color.black),bold'
property['style.keyword'] = 'fore:$(color.blue),bold'
property['style.label'] = 'fore:$(color.teal),bold'
property['style.number'] = 'fore:$(color.teal)'
property['style.operator'] = 'fore:$(color.black),bold'
property['style.regex'] = '$(style.string)'
property['style.string'] = 'fore:$(color.purple)'
property['style.preprocessor'] = 'fore:$(color.yellow)'
property['style.tag'] = 'fore:$(color.teal)'
property['style.type'] = 'fore:$(color.blue)'
property['style.variable'] = 'fore:$(color.black)'
property['style.whitespace'] = ''
property['style.embedded'] = 'fore:$(color.blue)'
property['style.identifier'] = '$(style.nothing)'

-- Predefined styles.
property['style.linenumber'] = 'back:#C0C0C0'
property['style.bracelight'] = 'fore:#0000FF,bold'
property['style.bracebad'] = 'fore:#FF0000,bold'
property['style.controlchar'] = '$(style.nothing)'
property['style.indentguide'] = 'fore:#C0C0C0,back:$(color.white)'
property['style.calltip'] = 'fore:$(color.white),back:#444444'
