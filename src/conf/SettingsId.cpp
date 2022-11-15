#include "SettingsId.h"

#include <QMap>

QString SettingsId::key(const Enum id) {
  return keys().value(id);
}

QMap<SettingsId::Enum, QString> SettingsId::keys() {
  static QMap<Enum, QString> keys;
  if (keys.isEmpty()) {
    initialize(keys);
  }
  return keys;
}

void SettingsId::initialize(QMap<SettingsId::Enum, QString>& keys) {
  keys[ShowAvatars] = "window/view/avatarsVisible";
}
