#include <QDialog>
#include "git/Signature.h"

class QLineEdit;
class QTextEdit;
class QCheckBox;
class QDateTimeEdit;

class AmendDialog : public QDialog {
public:
  AmendDialog(const git::Signature &author, const git::Signature &committer,
              const QString &commitMessage, QWidget *parent = nullptr);
  QString authorName() const;
  QString authorEmail() const;
  QDateTime authorCommitDate() const;
  bool editAuthorCommitDate() const;
  QString committerName() const;
  QString committerEmail() const;
  QDateTime committerCommitDate() const;
  bool editCommitterCommitDate() const;
  QString commitMessage() const;
  

private:
  QLineEdit *m_authorName;
  QLineEdit *m_authorEmail;
  QDateTimeEdit *m_authorCommitDate;
  QCheckBox *m_editAuthorCommitDate;
  QLineEdit *m_committerName;
  QLineEdit *m_committerEmail;
  QDateTimeEdit *m_committerCommitDate;
  QCheckBox *m_editCommitterCommitDate;
  QTextEdit *m_commitMessage;
};
