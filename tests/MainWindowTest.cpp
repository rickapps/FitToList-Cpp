#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QImage>
#include <QLabel>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest/QtTest>

#include "CanvasWidget.h"
#include "MainWindow.h"

namespace {
QString writeTestImage(QTemporaryDir &dir, int width, int height) {
    QImage image(width, height, QImage::Format_RGB32);
    image.fill(Qt::red);
    const QString path = dir.path() + "/photo.png";
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

// QFileDialog::getOpenFileName() blocks in its own event loop, so the only
// way to drive it from a test is to pre-arm a timer that fires once that
// loop starts, finds the now-modal dialog, and answers it.
void answerNextFileDialogWith(const QString &path) {
    QTimer::singleShot(0, [path] {
        auto *dialog = qobject_cast<QFileDialog *>(QApplication::activeModalWidget());
        QVERIFY(dialog);
        dialog->selectFile(path);
        QTest::keyClick(dialog, Qt::Key_Enter);
    });
}
}  // namespace

class MainWindowTest : public QObject {
    Q_OBJECT

private slots:
    void initialStateShowsPlaceholderTitleAndMessage() {
        MainWindow window;
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));

        QCOMPARE(window.windowTitle(), QString("FitToList"));
        auto *status = window.findChild<QLabel *>();
        QVERIFY(status);
        QCOMPARE(status->text(), QString("Open an image to begin."));
    }

    void openImageActionUpdatesTitleAndStatus() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = writeTestImage(dir, 200, 100);
        QVERIFY(!path.isEmpty());

        MainWindow window;
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));

        QAction *openAction = findActionByText(&window, "&Open Image...");
        QVERIFY(openAction);

        answerNextFileDialogWith(path);
        openAction->trigger();

        QVERIFY(window.windowTitle().startsWith("photo.png (200 x 100)"));
        auto *status = window.findChild<QLabel *>();
        QVERIFY(status);
        QCOMPARE(status->text(), QString("Ready."));
    }

    void cropActionAppliesCanvasSelectionToDocument() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = writeTestImage(dir, 200, 100);
        QVERIFY(!path.isEmpty());

        MainWindow window;
        window.resize(600, 500);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));

        answerNextFileDialogWith(path);
        findActionByText(&window, "&Open Image...")->trigger();

        auto *canvas = window.findChild<CanvasWidget *>();
        QVERIFY(canvas);

        // Drag out a selection covering roughly the canvas's top-left half,
        // in fractions of its actual (layout-dependent) size rather than
        // hardcoded pixels.
        const QSize size = canvas->size();
        const QPoint from(size.width() * 0.1, size.height() * 0.1);
        const QPoint to(size.width() * 0.6, size.height() * 0.6);
        QTest::mousePress(canvas, Qt::LeftButton, Qt::NoModifier, from);
        QTest::mouseMove(canvas, to);
        QTest::mouseRelease(canvas, Qt::LeftButton, Qt::NoModifier, to);
        QVERIFY(canvas->hasSelection());

        QAction *cropAction = findActionByText(&window, "&Crop to Selection");
        QVERIFY(cropAction);
        cropAction->trigger();

        QVERIFY(!canvas->hasSelection());
        QVERIFY(window.isWindowModified());
        QVERIFY(!window.windowTitle().startsWith("photo.png (200 x 100)"));
    }
};

QTEST_MAIN(MainWindowTest)
#include "MainWindowTest.moc"
