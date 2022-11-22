#include "SettingsId.h"

#include <QMap>

QString SettingsId::key(const Id id) {
  return keys().value(id);
}

QMap<SettingsId::Id, QString> SettingsId::keys() {
  static QMap<Id, QString> keys;
  if (keys.isEmpty()) {
    initialize(keys);
  }
  return keys;
}

void SettingsId::initialize(QMap<Id, QString>& keys) {
  keys[Id::ShowAvatars] = "window/view/avatarsVisible";
}
