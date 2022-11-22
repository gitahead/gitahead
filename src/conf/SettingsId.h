#ifndef SETTINGSID_H
#define SETTINGSID_H

#include <QObject>

class SettingsId {
  Q_GADGET

public:
  enum class Id {
    ShowAvatars,
  };
  Q_ENUM(Id)

  static QString key(Id id);

private:
  SettingsId() {}

  static QMap<Id, QString> keys();

  static void initialize(QMap<Id, QString>& keys);
};

#endif
