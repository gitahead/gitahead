#include "Setting.h"

void Setting::initialize(QMap<Id, QString> &keys) {
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
