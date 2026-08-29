#pragma once

#include <QDialog>
#include <QString>

class DirectoryTreeWidget;
class QLineEdit;

// Modal dialog for choosing both the source and processed folders at once.
// Ports FolderSelectionDialog: read-only fields above each tab always mirror
// whichever folder is highlighted in that tab's DirectoryTreeWidget, so
// what's on screen is always what a subsequent Save would use.
class FolderSelectionDialog : public QDialog {
    Q_OBJECT

public:
    FolderSelectionDialog(QWidget *parent, const QString &initialSource, const QString &initialTarget);

    // Valid only after exec() returns QDialog::Accepted.
    QString selectedSource() const { return sourcePath_; }
    QString selectedTarget() const { return targetPath_; }

private slots:
    void trySave();

private:
    DirectoryTreeWidget *sourceTree_;
    DirectoryTreeWidget *targetTree_;
    QString sourcePath_;
    QString targetPath_;
};
