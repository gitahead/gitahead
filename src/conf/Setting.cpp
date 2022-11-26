#include "Setting.h"

void Setting::initialize(QMap<Id, QString> &keys) {
  keys[Id::ShowAvatars] = "window/view/avatarsVisible";
}

void Prompt::initialize(QMap<PromptKind, QString> &keys) {
  const QString root("window/prompt/");
  keys[PromptKind::PromptMerge] = root + "merge";
  keys[PromptKind::PromptStash] = root + "stash";
  keys[PromptKind::PromptRevert] = root + "revert";
  keys[PromptKind::PromptCherryPick] = root + "cherrypick";
  keys[PromptKind::PromptDirectories] = root + "directories";
  keys[PromptKind::PromptLargeFiles] = root + "largeFiles";
}
