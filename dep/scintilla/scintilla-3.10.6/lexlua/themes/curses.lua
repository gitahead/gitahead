-- Copyright 2007-2019 Mitchell mitchell.att.foicica.com. See License.txt.
-- Curses theme for Lua lexers.
-- Contributions by Ana Balan.

local property = require('lexer').property

-- Normal colors.
property['color.black'] = '#000000'
property['color.red'] = '#800000'
property['color.green'] = '#008000'
property['color.yellow'] = '#808000'
property['color.blue'] = '#000080'
property['color.magenta'] = '#800080'
property['color.cyan'] = '#008080'
property['color.white'] = '#C0C0C0'

-- Light colors. (16 color terminals only.)
-- These only apply to 16 color terminals. For other terminals, set the
-- style's `bold` attribute to use the light color variant.
property['color.light_black'] = '#404040'
property['color.light_red'] = '#FF0000'
property['color.light_green'] = '#00FF00'
--property['color.light_yellow'] = '#FFFF00'
property['color.light_blue'] = '#0000FF'
property['color.light_magenta'] = '#FF00FF'
--property['color.light_cyan'] = '#0000FF'
property['color.light_white'] = '#FFFFFF'

-- Predefined styles.
property['style.default'] = 'fore:$(color.white),back:$(color.black)'
property['style.linenumber'] = ''
property['style.bracelight'] = 'fore:$(color.black),back:$(color.white)'
property['style.controlchar'] = ''
property['style.indentguide'] = ''
property['style.calltip'] = '$(style.default)'
property['style.folddisplaytext'] = 'fore:$(color.black),bold'

-- Token styles.
property['style.class'] = 'fore:$(color.yellow)'
property['style.comment'] = 'fore:$(color.black),bold'
property['style.constant'] = 'fore:$(color.red)'
property['style.embedded'] = '$(style.keyword),back:$(color.black)'
property['style.error'] = 'fore:$(color.red),bold'
property['style.function'] = 'fore:$(color.blue)'
property['style.identifier'] = ''
property['style.keyword'] = 'fore:$(color.white),bold'
property['style.label'] = 'fore:$(color.red),bold'
property['style.number'] = 'fore:$(color.cyan)'
property['style.operator'] = 'fore:$(color.yellow)'
property['style.preprocessor'] = 'fore:$(color.magenta)'
property['style.regex'] = 'fore:$(color.green),bold'
property['style.string'] = 'fore:$(color.green)'
property['style.type'] = 'fore:$(color.magenta),bold'
property['style.variable'] = 'fore:$(color.blue),bold'
property['style.whitespace'] = ''
