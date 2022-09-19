#include "AmendDialog.h"

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QHBoxLayout>

enum Row {
  AuthorName = 0,
  AuthorEmail,
  CommitterName,
  CommitterEmail,
  CommitMessage,

  Buttons
};

AmendDialog::AmendDialog(const git::Signature &author,
                         const git::Signature &committer,
                         const QString &commitMessage, QWidget *parent)
    : QDialog(parent) {

  auto *l = new QGridLayout();

  // committer
  // author
  // message

  auto *lAuthor = new QLabel(tr("Author name:"), this);
  m_authorName = new QLineEdit(author.name(), this);
  auto *lAuthorEmail = new QLabel(tr("Author email:"), this);
  m_authorEmail = new QLineEdit(author.email(), this);
  l->addWidget(lAuthor, Row::AuthorName, 0);
  l->addWidget(m_authorName, Row::AuthorName, 1);
  l->addWidget(lAuthorEmail, Row::AuthorEmail, 0);
  l->addWidget(m_authorEmail, Row::AuthorEmail, 1);

  auto *lCommitterName = new QLabel(tr("Committer name:"), this);
  m_committerName = new QLineEdit(committer.name(), this);
  auto *lCommitterEmail = new QLabel(tr("Committer email:"), this);
  m_committerEmail = new QLineEdit(committer.email(), this);
  l->addWidget(lCommitterName, Row::CommitterName, 0);
  l->addWidget(m_committerName, Row::CommitterName, 1);
  l->addWidget(lCommitterEmail, Row::CommitterEmail, 0);
  l->addWidget(m_committerEmail, Row::CommitterEmail, 1);

  auto *lMessage = new QLabel(tr("Message:"), this);
  m_commitMessage = new QTextEdit(commitMessage, this);
  l->addWidget(lMessage, Row::CommitMessage, 0);
  l->addWidget(m_commitMessage, Row::CommitMessage, 1, 2, 1);

  auto *ok = new QPushButton(tr("Amend"), this);
  auto *cancel = new QPushButton(tr("Cancel"), this);

  connect(ok, &QPushButton::clicked, this, &QDialog::accept);
  connect(cancel, &QPushButton::clicked, this, &QDialog::reject);

  auto *hl = new QHBoxLayout();
  hl->addWidget(cancel);
  hl->addWidget(ok);

  l->addLayout(hl, Buttons, 1);

  setLayout(l);
}

QString AmendDialog::authorName() const { return m_authorName->text(); }

QString AmendDialog::authorEmail() const { return m_authorEmail->text(); }

QString AmendDialog::committerName() const { return m_committerName->text(); }

QString AmendDialog::committerEmail() const { return m_committerEmail->text(); }

QString AmendDialog::commitMessage() const {
  return m_commitMessage->toPlainText();
}
