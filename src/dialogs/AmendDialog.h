#include <QDialog>
#include "git/Signature.h"

class QLineEdit;
class QTextEdit;

class AmendDialog: public QDialog {
public:
	AmendDialog(const git::Signature &author, const git::Signature &committer, const QString &commitMessage, QWidget *parent = nullptr);
	QString authorName() const;
	QString authorEmail() const;
	QString committerName() const;
	QString committerEmail() const;
	QString commitMessage() const;

private:
	QLineEdit* m_authorName;
	QLineEdit* m_authorEmail;
	QLineEdit* m_committerName;
	QLineEdit* m_committerEmail;
	QTextEdit* m_commitMessage;
};
