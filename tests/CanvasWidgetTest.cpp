#include <QImage>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "CanvasWidget.h"
#include "ImageDocument.h"

namespace {
// Writes a 200x100 opaque PNG and returns its path.
QString writeTestImage(QTemporaryDir &dir) {
    QImage image(200, 100, QImage::Format_RGB32);
    image.fill(Qt::red);
    const QString path = dir.path() + "/source.png";
    return image.save(path) ? path : QString();
}
}  // namespace

// Every test uses a 400x200 widget showing a 200x100 image, which fits it at
// exactly scale=2.0 with a (0,0) offset - chosen so every canvas<->image
// coordinate below is a round number.

class CanvasWidgetTest : public QObject {
    Q_OBJECT

private slots:
    void draggingNewSelectionThenCroppingAppliesToDocument() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = writeTestImage(dir);
        QVERIFY(!path.isEmpty());

        ImageDocument doc;
        QVERIFY(doc.load(path));
        CanvasWidget canvas;
        canvas.setDocument(&doc);
        canvas.resize(400, 200);
        canvas.show();
        QVERIFY(QTest::qWaitForWindowExposed(&canvas));

        // Canvas (100,50)->(300,150) at scale 2.0 is image-space (50,25)->(150,75).
        QTest::mousePress(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(100, 50));
        QTest::mouseMove(&canvas, QPoint(300, 150));
        QTest::mouseRelease(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(300, 150));

        QCOMPARE(canvas.selectionImageSize(), QSize(100, 50));
        QVERIFY(canvas.cropToSelection());
        QCOMPARE(doc.current().size(), QSize(100, 50));
        QVERIFY(!canvas.hasSelection());
    }

    void clickWithoutDraggingLeavesNoSelection() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = writeTestImage(dir);
        QVERIFY(!path.isEmpty());

        ImageDocument doc;
        QVERIFY(doc.load(path));
        CanvasWidget canvas;
        canvas.setDocument(&doc);
        canvas.resize(400, 200);
        canvas.show();
        QVERIFY(QTest::qWaitForWindowExposed(&canvas));

        QTest::mousePress(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(100, 50));
        QTest::mouseRelease(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(100, 50));

        QVERIFY(!canvas.hasSelection());
    }

    void smallDragGrowsSelectionToMinimumSize() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = writeTestImage(dir);
        QVERIFY(!path.isEmpty());

        ImageDocument doc;
        QVERIFY(doc.load(path));
        CanvasWidget canvas;
        canvas.setDocument(&doc);
        canvas.resize(400, 200);
        canvas.show();
        QVERIFY(QTest::qWaitForWindowExposed(&canvas));

        // 10 canvas px on each axis is a real drag (over the 4px click
        // threshold) but well under the 32 canvas-px minimum selection size
        // (a canvas-space constant, so edges stay grabbable regardless of
        // zoom), so it should grow away from the anchor to 32 canvas px =
        // 16 image px at this test's scale of 2.0.
        QTest::mousePress(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(100, 50));
        QTest::mouseMove(&canvas, QPoint(110, 60));
        QTest::mouseRelease(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(110, 60));

        QCOMPARE(canvas.selectionImageSize(), QSize(16, 16));
    }

    void clickingAwayFromStraightenHandlesDismissesTool() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = writeTestImage(dir);
        QVERIFY(!path.isEmpty());

        ImageDocument doc;
        QVERIFY(doc.load(path));
        CanvasWidget canvas;
        canvas.setDocument(&doc);
        canvas.resize(400, 200);
        canvas.show();
        QVERIFY(QTest::qWaitForWindowExposed(&canvas));

        canvas.startStraighten();
        QVERIFY(canvas.isStraightenActive());

        // Center of the canvas is nowhere near either line handle.
        QTest::mousePress(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(200, 100));
        QTest::mouseRelease(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(200, 100));

        QVERIFY(!canvas.isStraightenActive());
        QCOMPARE(doc.current().size(), QSize(200, 100));
        QVERIFY(!doc.isDirty());
    }

    void draggingStraightenHandleRotatesDocumentAndStaysActive() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = writeTestImage(dir);
        QVERIFY(!path.isEmpty());

        ImageDocument doc;
        QVERIFY(doc.load(path));
        CanvasWidget canvas;
        canvas.setDocument(&doc);
        canvas.resize(400, 200);
        canvas.show();
        QVERIFY(QTest::qWaitForWindowExposed(&canvas));

        canvas.startStraighten();
        // At angle 0, the line's right handle sits at (center.x + 0.35*dispW, center.y)
        // = (200 + 0.35*400, 100) = (340, 100).
        QTest::mousePress(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(340, 100));
        QTest::mouseMove(&canvas, QPoint(340, 150));
        QTest::mouseRelease(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(340, 150));

        QVERIFY(doc.isDirty());
        QVERIFY(doc.current().size() != QSize(200, 100));  // expand=True grew the canvas
        QVERIFY(canvas.isStraightenActive());               // tool stays open for fine-tuning

        const QSize afterFirstDrag = doc.current().size();
        canvas.cancelStraighten();
        QVERIFY(!canvas.isStraightenActive());
        QCOMPARE(doc.current().size(), afterFirstDrag);  // cancel doesn't revert a committed rotation
    }

    void finalizeStraightenCommitsPendingDragAndEndsSession() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = writeTestImage(dir);
        QVERIFY(!path.isEmpty());

        ImageDocument doc;
        QVERIFY(doc.load(path));
        CanvasWidget canvas;
        canvas.setDocument(&doc);
        canvas.resize(400, 200);
        canvas.show();
        QVERIFY(QTest::qWaitForWindowExposed(&canvas));

        canvas.startStraighten();
        // Press and move without releasing - a still-in-flight drag, the way
        // switching images mid-drag would find it.
        QTest::mousePress(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(340, 100));
        QTest::mouseMove(&canvas, QPoint(340, 150));
        QVERIFY(canvas.straightenAngle() != 0.0);

        canvas.finalizeStraighten();

        QVERIFY(!canvas.isStraightenActive());
        QVERIFY(doc.isDirty());
        QVERIFY(doc.current().size() != QSize(200, 100));
    }

    void doubleClickInsideSelectionEmitsProcessAndSaveRequested() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = writeTestImage(dir);
        QVERIFY(!path.isEmpty());

        ImageDocument doc;
        QVERIFY(doc.load(path));
        CanvasWidget canvas;
        canvas.setDocument(&doc);
        canvas.resize(400, 200);
        canvas.show();
        QVERIFY(QTest::qWaitForWindowExposed(&canvas));

        QTest::mousePress(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(100, 50));
        QTest::mouseMove(&canvas, QPoint(300, 150));
        QTest::mouseRelease(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(300, 150));
        QVERIFY(canvas.hasSelection());

        QSignalSpy spy(&canvas, &CanvasWidget::processAndSaveRequested);
        QTest::mouseDClick(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(200, 100));
        QCOMPARE(spy.count(), 1);
    }
};

QTEST_MAIN(CanvasWidgetTest)
#include "CanvasWidgetTest.moc"
