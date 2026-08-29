#include <QApplication>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest/QtTest>

#include "FolderSelectionDialog.h"

namespace {
QPushButton *saveButton(FolderSelectionDialog &dialog) {
    return dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Save);
}

// Dismisses the next critical QMessageBox this dialog pops up, so a rejected
// Save doesn't block the test waiting for a click.
void dismissNextMessageBox() {
    QTimer::singleShot(50, [] {
        if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
            box->close();
        }
    });
}
}  // namespace

class FolderSelectionDialogTest : public QObject {
    Q_OBJECT

private slots:
    void rejectsWhenSourceDoesNotExist() {
        QTemporaryDir target;
        QVERIFY(target.isValid());
        FolderSelectionDialog dialog(nullptr, "/no/such/source/folder", target.path());

        dismissNextMessageBox();
        saveButton(dialog)->click();

        QVERIFY(dialog.result() != static_cast<int>(QDialog::Accepted));
    }

    void rejectsWhenSourceAndTargetAreTheSame() {
        QTemporaryDir shared;
        QVERIFY(shared.isValid());
        FolderSelectionDialog dialog(nullptr, shared.path(), shared.path());

        dismissNextMessageBox();
        saveButton(dialog)->click();

        QVERIFY(dialog.result() != static_cast<int>(QDialog::Accepted));
    }

    void acceptsTwoDistinctExistingFolders() {
        QTemporaryDir source;
        QTemporaryDir target;
        QVERIFY(source.isValid() && target.isValid());
        FolderSelectionDialog dialog(nullptr, source.path(), target.path());

        saveButton(dialog)->click();

        QCOMPARE(dialog.result(), static_cast<int>(QDialog::Accepted));
        QCOMPARE(QFileInfo(dialog.selectedSource()).absoluteFilePath(), QFileInfo(source.path()).absoluteFilePath());
        QCOMPARE(QFileInfo(dialog.selectedTarget()).absoluteFilePath(), QFileInfo(target.path()).absoluteFilePath());
    }
};

QTEST_MAIN(FolderSelectionDialogTest)
#include "FolderSelectionDialogTest.moc"
