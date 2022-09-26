#include <QDialog>
#include "git/Signature.h"

class QLineEdit;
class QTextEdit;
class QCheckBox;

class AmendDialog : public QDialog {
public:
  AmendDialog(const git::Signature &author, const git::Signature &committer,
              const QString &commitMessage, QWidget *parent = nullptr);
  QString authorName() const;
  QString authorEmail() const;
  QString authorCommitDate() const;
  bool editAuthorCommitDate() const;
  QString committerName() const;
  QString committerEmail() const;
  QString committerCommitDate() const;
  bool editCommitterCommitDate() const;
  QString commitMessage() const;
  

private:
  QLineEdit *m_authorName;
  QLineEdit *m_authorEmail;
  QLineEdit *m_authorCommitDate;
  QCheckBox *m_editAuthorCommitDate;
  QLineEdit *m_committerName;
  QLineEdit *m_committerEmail;
  QLineEdit *m_committerCommitDate;
  QCheckBox *m_editCommitterCommitDate;
  QTextEdit *m_commitMessage;
};
