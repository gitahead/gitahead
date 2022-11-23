#include "Setting.h"

void Setting::initialize(QMap<Id, QString>& keys) {
  keys[Id::ShowAvatars] = "window/view/avatarsVisible";
}
