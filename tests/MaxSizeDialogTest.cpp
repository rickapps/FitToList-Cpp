#include <QApplication>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QtTest/QtTest>

#include "MaxSizeDialog.h"

namespace {
QPushButton *saveButton(QDialogButtonBox *buttons) {
    return buttons->button(QDialogButtonBox::Save);
}
}  // namespace

class MaxSizeDialogTest : public QObject {
    Q_OBJECT

private slots:
    void fieldsStartDisabledWhenNotEnabled() {
        MaxSizeDialog dialog(nullptr, /*enabled=*/false, 0, 0);
        auto *width = dialog.findChild<QLineEdit *>();
        QVERIFY(width);
        QVERIFY(!width->isEnabled());
    }

    void togglingCheckboxEnablesFields() {
        MaxSizeDialog dialog(nullptr, /*enabled=*/false, 0, 0);
        auto *checkbox = dialog.findChild<QCheckBox *>();
        QVERIFY(checkbox);
        checkbox->setChecked(true);

        const auto edits = dialog.findChildren<QLineEdit *>();
        QCOMPARE(edits.size(), 2);
        for (QLineEdit *edit : edits) {
            QVERIFY(edit->isEnabled());
        }
    }

    void rejectsNonNumericOrZeroWhenEnabled() {
        MaxSizeDialog dialog(nullptr, /*enabled=*/true, 0, 0);
        const auto edits = dialog.findChildren<QLineEdit *>();
        QCOMPARE(edits.size(), 2);
        edits[0]->setText("abc");
        edits[1]->setText("600");

        auto *buttons = dialog.findChild<QDialogButtonBox *>();
        QVERIFY(buttons);
        // Invalid input shows a critical QMessageBox, which would block the
        // test waiting for a click - close it via a deferred timer instead.
        QTimer::singleShot(50, [] {
            if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                box->close();
            }
        });
        saveButton(buttons)->click();

        QVERIFY(dialog.result() != static_cast<int>(QDialog::Accepted));  // trySave() must not have reached accept()
    }

    void acceptsPositiveIntegersAndReportsThem() {
        MaxSizeDialog dialog(nullptr, /*enabled=*/true, 0, 0);
        const auto edits = dialog.findChildren<QLineEdit *>();
        QCOMPARE(edits.size(), 2);
        edits[0]->setText("800");
        edits[1]->setText("600");

        auto *buttons = dialog.findChild<QDialogButtonBox *>();
        QVERIFY(buttons);
        saveButton(buttons)->click();

        QCOMPARE(dialog.result(), static_cast<int>(QDialog::Accepted));
        QVERIFY(dialog.enabled());
        QCOMPARE(dialog.maxWidth(), 800);
        QCOMPARE(dialog.maxHeight(), 600);
    }

    void disabledStateSkipsValidationEvenWithEmptyFields() {
        MaxSizeDialog dialog(nullptr, /*enabled=*/false, 0, 0);
        auto *buttons = dialog.findChild<QDialogButtonBox *>();
        QVERIFY(buttons);
        saveButton(buttons)->click();

        QCOMPARE(dialog.result(), static_cast<int>(QDialog::Accepted));
        QVERIFY(!dialog.enabled());
    }
};

QTEST_MAIN(MaxSizeDialogTest)
#include "MaxSizeDialogTest.moc"
