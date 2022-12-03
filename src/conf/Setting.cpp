#include "Setting.h"

void Setting::initialize(QMap<Id, QString> &keys) {
  keys[Id::FontFamily] = "editor/font/family";
  keys[Id::FontSize] = "editor/font/size";
  keys[Id::UseTabsForIndent] = "editor/indent/tabs";
  keys[Id::IndentWidth] = "editor/indent/width";
  keys[Id::TabWidth] = "editor/indent/tabwidth";
  keys[Id::ShowHeatmapInBlameMargin] = "editor/blame/heatmap";
  keys[Id::ShowAvatars] = "window/view/avatarsVisible";
}

void Prompt::initialize(QMap<Kind, QString> &keys) {
  const QString root("window/prompt/");
  keys[Kind::Merge] = root + "merge";
  keys[Kind::Stash] = root + "stash";
  keys[Kind::Revert] = root + "revert";
  keys[Kind::CherryPick] = root + "cherrypick";
  keys[Kind::Directories] = root + "directories";
  keys[Kind::LargeFiles] = root + "largeFiles";
}
