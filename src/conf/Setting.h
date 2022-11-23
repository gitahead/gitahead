#ifndef SETTING_H
#define SETTING_H

#include <QObject>
#include <QMap>

template<class T>
class SettingsTempl {
public:
  template<typename TId>
  static QString key(const TId id) {
    return keys<TId>().value(id);
  }

private:
  template<typename TId>
  static QMap<TId, QString> keys() {
    static QMap<TId, QString> keys;
    if (keys.isEmpty()) {
      T::initialize(keys);
    }
    return keys;
  }
};

class Setting : public SettingsTempl<Setting> {
  Q_GADGET

public:
  enum class Id {
    ShowAvatars,
  };
  Q_ENUM(Id)

  static void initialize(QMap<Id, QString>& keys);

private:
  Setting() {}
};

class Prompt : public SettingsTempl<Prompt> {

public:
  enum class PromptKind {
    PromptStash,
    PromptMerge,
    PromptRevert,
    PromptCherryPick,
    PromptDirectories,
    PromptLargeFiles
  };

  static void initialize(QMap<PromptKind, QString>& keys);

};

#endif
