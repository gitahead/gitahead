//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDir>
#include <QString>
#include <QVariant>

class Settings : public QObject
{
  Q_OBJECT

public:
  enum PromptKind
  {
    PromptStash,
    PromptMerge,
    PromptRevert,
    PromptCherryPick
  };

  QString group() const;
  void beginGroup(const QString &prefix);
  void endGroup();

  QVariant value(const QString &key) const;
  QVariant defaultValue(const QString &key) const;
  void setValue(const QString &key, const QVariant &value);

  // Look up lexer name by file name.
  QString lexer(const QString &filename);
  QString kind(const QString &filename);

  // prompt dialogs
  bool prompt(PromptKind kind) const;
  void setPrompt(PromptKind kind, bool prompt);
  QString promptDescription(PromptKind kind) const;

  static QDir appDir();
  static QDir docDir();
  static QDir confDir();
  static QDir lexerDir();
  static QDir themesDir();
  static QDir pluginsDir();

  static QDir userDir();
  static QString locate(const QString &file);

  static QDir tempDir();

  static Settings *instance();

signals:
  void settingsChanged();

private:
  Settings(QObject *parent = nullptr);

  QStringList mGroup;
  QVariantMap mDefaults;
  QVariantMap mCurrentMap;
};

#endif
