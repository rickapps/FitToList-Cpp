#include <QAction>
#include <QApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QImage>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>
#include <QTimer>
#include <QToolButton>
#include <QtTest/QtTest>
#include <memory>

#include "CanvasWidget.h"
#include "DirectoryTreeWidget.h"
#include "FolderSelectionDialog.h"
#include "ImageTreeWidget.h"
#include "MainWindow.h"

namespace {
QString writeTestImage(const QString &dirPath, const QString &name, int width, int height) {
    QImage image(width, height, QImage::Format_RGB32);
    image.fill(Qt::red);
    const QString path = dirPath + "/" + name;
    return image.save(path) ? path : QString();
}

QAction *findActionByText(QWidget *root, const QString &text) {
    for (QAction *action : root->findChildren<QAction *>()) {
        if (action->text() == text) {
            return action;
        }
    }
    return nullptr;
}

// Drives MainWindow's "Select Folders..." action end-to-end: triggers it,
// then (once its modal FolderSelectionDialog is up) points both of its
// DirectoryTreeWidgets at source/target and clicks Save.
void selectFoldersVia(MainWindow &window, const QString &source, const QString &target) {
    QTimer::singleShot(0, [source, target] {
        auto *dialog = qobject_cast<FolderSelectionDialog *>(QApplication::activeModalWidget());
        QVERIFY(dialog);
        auto *sourceTree = dialog->findChild<DirectoryTreeWidget *>("sourceTree");
        auto *targetTree = dialog->findChild<DirectoryTreeWidget *>("targetTree");
        QVERIFY(sourceTree);
        QVERIFY(targetTree);
        sourceTree->setCurrentPath(source);
        targetTree->setCurrentPath(target);
        auto *buttons = dialog->findChild<QDialogButtonBox *>();
        QVERIFY(buttons);
        buttons->button(QDialogButtonBox::Save)->click();
    });
    QAction *action = findActionByText(&window, "&Select Folders...");
    QVERIFY(action);
    action->trigger();
}

// Clicks `role` on the next QMessageBox this window pops up, once its nested
// event loop starts - for confirmation dialogs (Save/Overwrite/Unsaved
// Changes prompts) that would otherwise block the test.
void answerNextMessageBoxWith(QMessageBox::StandardButton role) {
    QTimer::singleShot(0, [role] {
        auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        QVERIFY(box);
        if (auto *button = box->button(role)) {
            button->click();
        }
    });
}
}  // namespace

class MainWindowTest : public QObject {
    Q_OBJECT

private slots:
    // Redirects QSettings to a fresh throwaway location for every test
    // function, so MainWindow's persisted config never touches the real
    // user's settings and tests don't see each other's leftover state.
    void init() {
        settingsDir_ = std::make_unique<QTemporaryDir>();
        QVERIFY(settingsDir_->isValid());
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDir_->path());
        QSettings::setDefaultFormat(QSettings::IniFormat);
    }

    void selectFolders_populatesTreeAndAutoLoadsFirstImage() {
        QTemporaryDir source;
        QTemporaryDir target;
        QVERIFY(source.isValid() && target.isValid());
        QVERIFY(!writeTestImage(source.path(), "a.png", 150, 80).isEmpty());

        MainWindow window;
        window.resize(700, 500);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));

        selectFoldersVia(window, source.path(), target.path());

        QVERIFY(window.windowTitle().startsWith("a.png (150 x 80)"));
        auto *tree = window.findChild<ImageTreeWidget *>();
        QVERIFY(tree);
        QCOMPARE(tree->topLevelItemCount(), 1);
    }

    void cropThenSave_writesFileAndAddsProcessedChildNode() {
        QTemporaryDir source;
        QTemporaryDir target;
        QVERIFY(source.isValid() && target.isValid());
        QVERIFY(!writeTestImage(source.path(), "a.png", 200, 100).isEmpty());

        MainWindow window;
        window.resize(700, 500);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        selectFoldersVia(window, source.path(), target.path());

        auto *canvas = window.findChild<CanvasWidget *>();
        QVERIFY(canvas);
        const QSize size = canvas->size();
        QTest::mousePress(canvas, Qt::LeftButton, Qt::NoModifier, QPoint(size.width() * 0.1, size.height() * 0.1));
        QTest::mouseMove(canvas, QPoint(size.width() * 0.6, size.height() * 0.6));
        QTest::mouseRelease(canvas, Qt::LeftButton, Qt::NoModifier,
                             QPoint(size.width() * 0.6, size.height() * 0.6));
        QVERIFY(canvas->hasSelection());

        findActionByText(&window, "&Crop to Selection")->trigger();
        findActionByText(&window, "&Save")->trigger();

        QVERIFY(!window.isWindowModified());
        const QStringList written = QDir(target.path()).entryList(QDir::Files);
        QCOMPARE(written, QStringList({"a_00.png"}));

        auto *tree = window.findChild<ImageTreeWidget *>();
        QCOMPARE(tree->topLevelItem(0)->childCount(), 1);
        QCOMPARE(tree->topLevelItem(0)->child(0)->text(0), QString("a_00.png"));
    }

    void processAndSave_advancesToNextSourceImage() {
        QTemporaryDir source;
        QTemporaryDir target;
        QVERIFY(source.isValid() && target.isValid());
        QVERIFY(!writeTestImage(source.path(), "a.png", 100, 100).isEmpty());
        QVERIFY(!writeTestImage(source.path(), "b.png", 100, 100).isEmpty());

        MainWindow window;
        window.resize(700, 500);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        selectFoldersVia(window, source.path(), target.path());
        QVERIFY(window.windowTitle().startsWith("a.png"));

        findActionByText(&window, "&Process && Save")->trigger();

        QVERIFY(window.windowTitle().startsWith("b.png"));
        QVERIFY(QDir(target.path()).entryList(QDir::Files).contains("a_00.png"));
    }

    void switchingTreeSelectionWithUnsavedCropPromptsAndDiscardsOnNo() {
        QTemporaryDir source;
        QTemporaryDir target;
        QVERIFY(source.isValid() && target.isValid());
        QVERIFY(!writeTestImage(source.path(), "a.png", 100, 100).isEmpty());
        QVERIFY(!writeTestImage(source.path(), "b.png", 100, 100).isEmpty());

        MainWindow window;
        window.resize(700, 500);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        selectFoldersVia(window, source.path(), target.path());

        auto *canvas = window.findChild<CanvasWidget *>();
        const QSize size = canvas->size();
        QTest::mousePress(canvas, Qt::LeftButton, Qt::NoModifier, QPoint(size.width() * 0.1, size.height() * 0.1));
        QTest::mouseMove(canvas, QPoint(size.width() * 0.6, size.height() * 0.6));
        QTest::mouseRelease(canvas, Qt::LeftButton, Qt::NoModifier,
                             QPoint(size.width() * 0.6, size.height() * 0.6));
        QVERIFY(canvas->hasSelection());

        auto *tree = window.findChild<ImageTreeWidget *>();
        answerNextMessageBoxWith(QMessageBox::No);  // "discard"
        tree->setCurrentItem(tree->topLevelItem(1));

        QVERIFY(window.windowTitle().startsWith("b.png"));
        QVERIFY(!canvas->hasSelection());
    }

    void helpMenuHasUserGuideAndAboutActions() {
        MainWindow window;
        QVERIFY(findActionByText(&window, "User Guide"));
        QVERIFY(findActionByText(&window, "About FitToList"));
    }

    void toolbarSaveButtonSharesActionWithMenuAndSavesFile() {
        QTemporaryDir source;
        QTemporaryDir target;
        QVERIFY(source.isValid() && target.isValid());
        QVERIFY(!writeTestImage(source.path(), "a.png", 200, 100).isEmpty());

        MainWindow window;
        window.resize(700, 500);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        selectFoldersVia(window, source.path(), target.path());

        auto *canvas = window.findChild<CanvasWidget *>();
        const QSize size = canvas->size();
        QTest::mousePress(canvas, Qt::LeftButton, Qt::NoModifier, QPoint(size.width() * 0.1, size.height() * 0.1));
        QTest::mouseMove(canvas, QPoint(size.width() * 0.6, size.height() * 0.6));
        QTest::mouseRelease(canvas, Qt::LeftButton, Qt::NoModifier,
                             QPoint(size.width() * 0.6, size.height() * 0.6));
        findActionByText(&window, "&Crop to Selection")->trigger();

        QToolButton *saveButton = nullptr;
        for (QToolButton *button : window.findChildren<QToolButton *>()) {
            if (button->defaultAction() && button->defaultAction()->text() == "&Save") {
                saveButton = button;
                break;
            }
        }
        QVERIFY(saveButton);
        saveButton->click();

        QVERIFY(!window.isWindowModified());
        QCOMPARE(QDir(target.path()).entryList(QDir::Files), QStringList({"a_00.png"}));
    }

private:
    std::unique_ptr<QTemporaryDir> settingsDir_;
};

QTEST_MAIN(MainWindowTest)
#include "MainWindowTest.moc"
