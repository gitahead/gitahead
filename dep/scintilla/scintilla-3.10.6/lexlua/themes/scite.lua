-- Copyright 2006-2019 Mitchell mitchell.att.foicica.com. See License.txt.
-- SciTE theme for Lua lexers.

local property = require('lexer').property

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
property['font'], property['fontsize'] = 'Monospace', 11
if WIN32 then
  property['font'] = 'Courier New'
elseif OSX then
  property['font'], property['fontsize'] = 'Monaco', 12
end

-- Predefined styles.
property['style.default'] = 'font:$(font),size:$(fontsize),'..
                            'fore:$(color.black),back:$(color.white)'
property['style.linenumber'] = 'back:#C0C0C0'
property['style.bracelight'] = 'fore:#0000FF,bold'
property['style.bracebad'] = 'fore:#FF0000,bold'
property['style.controlchar'] = ''
property['style.indentguide'] = 'fore:#C0C0C0,back:$(color.white)'
property['style.calltip'] = 'fore:$(color.white),back:#444444'
property['style.folddisplaytext'] = ''

-- Token styles.
property['style.class'] = 'fore:$(color.black),bold'
property['style.comment'] = 'fore:$(color.green)'
property['style.constant'] = 'fore:$(color.teal),bold'
property['style.embedded'] = 'fore:$(color.blue)'
property['style.error'] = 'fore:$(color.red)'
property['style.function'] = 'fore:$(color.black),bold'
property['style.identifier'] = ''
property['style.keyword'] = 'fore:$(color.blue),bold'
property['style.label'] = 'fore:$(color.teal),bold'
property['style.number'] = 'fore:$(color.teal)'
property['style.operator'] = 'fore:$(color.black),bold'
property['style.preprocessor'] = 'fore:$(color.yellow)'
property['style.regex'] = '$(style.string)'
property['style.string'] = 'fore:$(color.purple)'
property['style.type'] = 'fore:$(color.blue)'
property['style.variable'] = 'fore:$(color.black)'
property['style.whitespace'] = ''
