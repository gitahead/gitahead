#include "Test.h"

#include "conf/SettingsId.h"

using namespace QTest;

class TestSettingsId : public QObject {
  Q_OBJECT

private slots:
  void defines_a_non_empty_settings_key_for_each_id();
  void defines_each_settings_key_only_once();

private:
  QList<SettingsId::Id> ids() const;
};

void TestSettingsId::defines_a_non_empty_settings_key_for_each_id() {
  QMetaEnum metaEnum = QMetaEnum::fromType<SettingsId::Id>();

  foreach (const SettingsId::Id id, ids()) {
    const QString settingsKey = SettingsId::key(id);

    QVERIFY2(!settingsKey.isEmpty(),
             qPrintable(QString("no settings key defined for %1::%2::%3").arg(
                            metaEnum.scope(), metaEnum.name(), metaEnum.valueToKey(static_cast<int>(id)))));
  }
}

void TestSettingsId::defines_each_settings_key_only_once() {
  QStringList settingsKeys;
  foreach (const SettingsId::Id id, ids()) {
    const QString settingsKey = SettingsId::key(id);

    QVERIFY2(!settingsKeys.contains(settingsKey),
             qPrintable(QString("the settings key '%1' is used for multiple settings ids").arg(settingsKey)));

    settingsKeys << settingsKey;
  }
}

QList<SettingsId::Id> TestSettingsId::ids() const {
  QList<SettingsId::Id> ids;

  QMetaEnum metaEnum = QMetaEnum::fromType<SettingsId::Id>();
  for (int i=0; i<metaEnum.keyCount(); i++) {
    ids << static_cast<SettingsId::Id>(metaEnum.value(i));
  }

  return ids;
}

TEST_MAIN(TestSettingsId)

#include "SettingsId.moc"
