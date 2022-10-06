#include <QDialog>
#include "git/Signature.h"

class QLineEdit;
class QTextEdit;
class QCheckBox;
class QDateTimeEdit;
class DateSelectionGroupWidget;
class QLabel;

class AmendDialog : public QDialog {
public:
  enum class SelectedDateTimeType { Current, Manual, Original };

  AmendDialog(const git::Signature &author, const git::Signature &committer,
              const QString &commitMessage, QWidget *parent = nullptr);
  QString authorName() const;
  QString authorEmail() const;
  QDateTime authorCommitDate() const;
  SelectedDateTimeType authorCommitDateType() const;
  QString committerName() const;
  QString committerEmail() const;
  QDateTime committerCommitDate() const;
  SelectedDateTimeType committerCommitDateType() const;
  QString commitMessage() const;

private slots:
  void authorDateTimeTypeChanged(const SelectedDateTimeType type);
  void committerDateTimeTypeChanged(const SelectedDateTimeType type);

private:
  QLineEdit *m_authorName;
  QLineEdit *m_authorEmail;
  QDateTimeEdit *m_authorCommitDate;
  QLabel *m_lAuthorCommitDate;
  DateSelectionGroupWidget *m_authorCommitDateType;
  QLineEdit *m_committerName;
  QLineEdit *m_committerEmail;
  QDateTimeEdit *m_committerCommitDate;
  QLabel *m_lCommitterCommitDate;
  DateSelectionGroupWidget *m_committerCommitDateType;
  QTextEdit *m_commitMessage;

  git::Signature m_author;
  git::Signature m_committer;
};
