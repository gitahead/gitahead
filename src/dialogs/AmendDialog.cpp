#include "AmendDialog.h"

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QDateTime>
#include <QDateTimeEdit>

enum Row {
  AuthorName = 0,
  AuthorEmail,
  EditAuthorCommitDate,
  AuthorCommitDate,
  CommitterName,
  CommitterEmail,
  EditCommitterCommitDate,
  CommitterCommitDate,
  CommitMessage,

  Buttons
};

AmendDialog::AmendDialog(const git::Signature &author,
                         const git::Signature &committer,
                         const QString &commitMessage, QWidget *parent)
    : QDialog(parent) {

  auto *l = new QGridLayout();

  // author
  // committer
  // message

  auto *lAuthor = new QLabel(tr("Author name:"), this);
  m_authorName = new QLineEdit(author.name(), this);
  auto *lAuthorEmail = new QLabel(tr("Author email:"), this);
  m_authorEmail = new QLineEdit(author.email(), this);
  auto *lAuthorCommitDate = new QLabel(tr("Author commit date:"), this);
  m_authorCommitDate = new QDateTimeEdit(author.date(), this);
  m_editAuthorCommitDate =
      new QCheckBox(tr("Manually set author commit date?"), this);
  l->addWidget(lAuthor, Row::AuthorName, 0);
  l->addWidget(m_authorName, Row::AuthorName, 1);
  l->addWidget(lAuthorEmail, Row::AuthorEmail, 0);
  l->addWidget(m_authorEmail, Row::AuthorEmail, 1);
  l->addWidget(m_editAuthorCommitDate, Row::EditAuthorCommitDate, 0);
  l->addWidget(lAuthorCommitDate, Row::AuthorCommitDate, 0);
  l->addWidget(m_authorCommitDate, Row::AuthorCommitDate, 1);

  auto *lCommitterName = new QLabel(tr("Committer name:"), this);
  m_committerName = new QLineEdit(committer.name(), this);
  auto *lCommitterEmail = new QLabel(tr("Committer email:"), this);
  m_committerEmail = new QLineEdit(committer.email(), this);
  auto *lCommitterCommitDate = new QLabel(tr("Committer commit date:"), this);
  m_committerCommitDate = new QDateTimeEdit(committer.date(), this);
  m_editCommitterCommitDate =
      new QCheckBox(tr("Manually set committer commit date?"), this);
  l->addWidget(lCommitterName, Row::CommitterName, 0);
  l->addWidget(m_committerName, Row::CommitterName, 1);
  l->addWidget(lCommitterEmail, Row::CommitterEmail, 0);
  l->addWidget(m_committerEmail, Row::CommitterEmail, 1);
  l->addWidget(m_editCommitterCommitDate, Row::EditCommitterCommitDate, 0);
  l->addWidget(lCommitterCommitDate, Row::CommitterCommitDate, 0);
  l->addWidget(m_committerCommitDate, Row::CommitterCommitDate, 1);

  auto *lMessage = new QLabel(tr("Message:"), this);
  m_commitMessage = new QTextEdit(commitMessage, this);
  l->addWidget(lMessage, Row::CommitMessage, 0);
  l->addWidget(m_commitMessage, Row::CommitMessage, 1, 2, 1);

  auto *ok = new QPushButton(tr("Amend"), this);
  auto *cancel = new QPushButton(tr("Cancel"), this);

  connect(ok, &QPushButton::clicked, this, &QDialog::accept);
  connect(cancel, &QPushButton::clicked, this, &QDialog::reject);

  auto chbStateChangedHandler = [](QCheckBox *chb, QWidget *l) {
    auto enable = chb->checkState() == Qt::Checked;
    l->setEnabled(enable);
  };

  connect(m_editAuthorCommitDate, &QCheckBox::stateChanged, this,
          [&, chbStateChangedHandler]() {
            chbStateChangedHandler(m_editAuthorCommitDate, m_authorCommitDate);
          });

  connect(m_editCommitterCommitDate, &QCheckBox::stateChanged, this,
          [&, chbStateChangedHandler]() {
            chbStateChangedHandler(m_editCommitterCommitDate,
                                   m_committerCommitDate);
          });

  chbStateChangedHandler(m_editAuthorCommitDate, m_authorCommitDate);
  chbStateChangedHandler(m_editCommitterCommitDate, m_committerCommitDate);

  auto *hl = new QHBoxLayout();
  hl->addWidget(cancel);
  hl->addWidget(ok);

  l->addLayout(hl, Buttons, 1);

  setLayout(l);
}

QString AmendDialog::authorName() const { return m_authorName->text(); }

QString AmendDialog::authorEmail() const { return m_authorEmail->text(); }

QDateTime AmendDialog::authorCommitDate() const {
  return m_authorCommitDate->dateTime();
}

bool AmendDialog::editAuthorCommitDate() const {
  return m_editAuthorCommitDate->checkState() == Qt::Checked;
}

QString AmendDialog::committerName() const { return m_committerName->text(); }

QString AmendDialog::committerEmail() const { return m_committerEmail->text(); }

QDateTime AmendDialog::committerCommitDate() const {
  return m_committerCommitDate->dateTime();
}

bool AmendDialog::editCommitterCommitDate() const {
  return m_editAuthorCommitDate->checkState() == Qt::Checked;
}

QString AmendDialog::commitMessage() const {
  return m_commitMessage->toPlainText();
}
