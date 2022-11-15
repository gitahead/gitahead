#ifndef SETTINGSID_H
#define SETTINGSID_H

#include <QObject>

class SettingsId {
  Q_GADGET

public:
  enum Enum {
    ShowAvatars,
  };
  Q_ENUM(Enum)

  static QString key(Enum id);

private:
  SettingsId() {}

  static QMap<Enum, QString> keys();

  static void initialize(QMap<SettingsId::Enum, QString>& keys);
};

#endif
