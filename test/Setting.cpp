#include "Test.h"

#include "conf/Setting.h"

using namespace QTest;

class TestSetting : public QObject {
  Q_OBJECT

private slots:
  void defines_a_non_empty_settings_key_for_each_id();
  void defines_each_settings_key_only_once();

private:
  QList<Setting::Id> ids() const;
};

void TestSetting::defines_a_non_empty_settings_key_for_each_id() {
  QMetaEnum metaEnum = QMetaEnum::fromType<Setting::Id>();

  foreach (const Setting::Id id, ids()) {
    const QString settingsKey = Setting::key(id);

    QVERIFY2(!settingsKey.isEmpty(),
             qPrintable(QString("no settings key defined for %1::%2::%3")
                            .arg(metaEnum.scope(), metaEnum.name(),
                                 metaEnum.valueToKey(static_cast<int>(id)))));
  }
}

void TestSetting::defines_each_settings_key_only_once() {
  QStringList settingsKeys;
  foreach (const Setting::Id id, ids()) {
    const QString settingsKey = Setting::key(id);

    QVERIFY2(
        !settingsKeys.contains(settingsKey),
        qPrintable(
            QString("the settings key '%1' is used for multiple settings ids")
                .arg(settingsKey)));

    settingsKeys << settingsKey;
  }
}

QList<Setting::Id> TestSetting::ids() const {
  QList<Setting::Id> ids;

  QMetaEnum metaEnum = QMetaEnum::fromType<Setting::Id>();
  for (int i = 0; i < metaEnum.keyCount(); i++) {
    ids << static_cast<Setting::Id>(metaEnum.value(i));
  }

  return ids;
}

TEST_MAIN(TestSetting)

#include "Setting.moc"
