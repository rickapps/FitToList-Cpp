#include "FolderSelectionDialog.h"

#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QTabWidget>
#include <QVBoxLayout>

#include "DirectoryTreeWidget.h"

FolderSelectionDialog::FolderSelectionDialog(QWidget *parent, const QString &initialSource,
                                              const QString &initialTarget)
    : QDialog(parent), sourcePath_(initialSource), targetPath_(initialTarget) {
    setWindowTitle("Select Folders");
    resize(480, 560);

    auto *layout = new QVBoxLayout(this);

    auto *displayLayout = new QGridLayout;
    auto *sourceEdit = new QLineEdit(initialSource, this);
    sourceEdit->setReadOnly(true);
    auto *targetEdit = new QLineEdit(initialTarget, this);
    targetEdit->setReadOnly(true);
    displayLayout->addWidget(new QLabel("Source:", this), 0, 0);
    displayLayout->addWidget(sourceEdit, 0, 1);
    displayLayout->addWidget(new QLabel("Processed:", this), 1, 0);
    displayLayout->addWidget(targetEdit, 1, 1);
    layout->addLayout(displayLayout);

    auto *tabs = new QTabWidget(this);
    sourceTree_ = new DirectoryTreeWidget(this);
    sourceTree_->setObjectName("sourceTree");
    sourceTree_->setCurrentPath(initialSource);
    targetTree_ = new DirectoryTreeWidget(this);
    targetTree_->setObjectName("targetTree");
    targetTree_->setCurrentPath(!initialTarget.isEmpty() ? initialTarget : initialSource);
    tabs->addTab(sourceTree_, "Source");
    tabs->addTab(targetTree_, "Processed");
    layout->addWidget(tabs, 1);

    connect(sourceTree_, &DirectoryTreeWidget::pathChanged, sourceEdit, &QLineEdit::setText);
    connect(sourceTree_, &DirectoryTreeWidget::pathChanged, this, [this](const QString &path) { sourcePath_ = path; });
    connect(targetTree_, &DirectoryTreeWidget::pathChanged, targetEdit, &QLineEdit::setText);
    connect(targetTree_, &DirectoryTreeWidget::pathChanged, this, [this](const QString &path) { targetPath_ = path; });

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &FolderSelectionDialog::trySave);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void FolderSelectionDialog::trySave() {
    if (sourcePath_.isEmpty() || !QDir(sourcePath_).exists()) {
        QMessageBox::critical(this, "Select Folders", "Please select a valid source folder.");
        return;
    }
    if (targetPath_.isEmpty() || !QDir(targetPath_).exists()) {
        QMessageBox::critical(this, "Select Folders", "Please select a valid processed folder.");
        return;
    }
    if (QFileInfo(sourcePath_).absoluteFilePath() == QFileInfo(targetPath_).absoluteFilePath()) {
        QMessageBox::critical(this, "Select Folders", "Source folder cannot be the same as the processed folder.");
        return;
    }
    accept();
}
