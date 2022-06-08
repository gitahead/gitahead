# add release notes to the appdata file
file(READ "${CHANGELOG_HTML}" HTML_CHANGELOGS)
string(REGEX REPLACE "<h4>([A-Fa-f0-9]*)<\\/h4>" "\\1" RELEASES ${HTML_CHANGELOGS}) # h4 is unknow to appdata
string(REPLACE "\n" "\n\t" RELEASES ${RELEASES}) # add tabulator
string(REGEX REPLACE "<h3>(v[1-9]\\.[0-9]\\.[0-9]) - ([0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9])<\\/h3>" "<release version='\\1' date='\\2'>\n\t<description>" RELEASES ${RELEASES})
string(REGEX REPLACE "<hr \\/>" "</description>\n\t</release>" RELEASES ${RELEASES})
configure_file(${APPDATA_CONF} ${APPDATA})
