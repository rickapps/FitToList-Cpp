#include <QImage>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "ImageDocument.h"

namespace {
// Writes a solid-color opaque PNG and returns its path.
QString writeTestImage(QTemporaryDir &dir, int width, int height) {
    QImage image(width, height, QImage::Format_RGB32);
    image.fill(Qt::red);
    const QString path = dir.path() + "/source.png";
    return image.save(path) ? path : QString();
}
}  // namespace

class ImageDocumentTest : public QObject {
    Q_OBJECT

private slots:
    void clear_dropsImageEntirely() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = writeTestImage(dir, 100, 60);
        QVERIFY(!path.isEmpty());

        ImageDocument doc;
        QVERIFY(doc.load(path));
        doc.clear();
        QVERIFY(doc.current().isNull());
        QVERIFY(doc.original().isNull());
        QVERIFY(!doc.isDirty());
    }

    void saveTo_clearsDirtyFlagOnSuccess() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = writeTestImage(dir, 100, 60);
        QVERIFY(!path.isEmpty());

        ImageDocument doc;
        QVERIFY(doc.load(path));
        doc.rotateRight();
        QVERIFY(doc.isDirty());

        const QString outPath = dir.path() + "/out.png";
        QVERIFY(doc.saveTo(outPath, doc.current().size()));
        QVERIFY(!doc.isDirty());
    }

    void crop_clampsToImageBoundsAndMarksDirty() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = writeTestImage(dir, 100, 60);
        QVERIFY(!path.isEmpty());

        ImageDocument doc;
        QVERIFY(doc.load(path));
        QVERIFY(!doc.isDirty());

        QVERIFY(doc.crop(QRect(-10, -10, 50, 40)));
        QCOMPARE(doc.current().size(), QSize(40, 30));
        QVERIFY(doc.isDirty());
    }

    void crop_tooSmallIsNoOp() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = writeTestImage(dir, 100, 60);
        QVERIFY(!path.isEmpty());

        ImageDocument doc;
        QVERIFY(doc.load(path));
        QVERIFY(!doc.crop(QRect(0, 0, 1, 1)));
        QCOMPARE(doc.current().size(), QSize(100, 60));
        QVERIFY(!doc.isDirty());
    }

    void reset_restoresOriginalAndClearsDirty() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = writeTestImage(dir, 100, 60);
        QVERIFY(!path.isEmpty());

        ImageDocument doc;
        QVERIFY(doc.load(path));
        QVERIFY(doc.crop(QRect(0, 0, 40, 30)));
        doc.reset();
        QCOMPARE(doc.current().size(), QSize(100, 60));
        QVERIFY(!doc.isDirty());
    }

    void rotate_swapsDimensions() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = writeTestImage(dir, 100, 60);
        QVERIFY(!path.isEmpty());

        ImageDocument doc;
        QVERIFY(doc.load(path));
        doc.rotateRight();
        QCOMPARE(doc.current().size(), QSize(60, 100));
    }

    void plannedSaveSize_capsOnlyWhenItWouldShrink() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = writeTestImage(dir, 200, 100);
        QVERIFY(!path.isEmpty());

        ImageDocument doc;
        QVERIFY(doc.load(path));

        QCOMPARE(doc.plannedSaveSize(false, 0, 0), QSize(200, 100));
        QCOMPARE(doc.plannedSaveSize(true, 1000, 1000), QSize(200, 100));
        QCOMPARE(doc.plannedSaveSize(true, 100, 100), QSize(100, 50));
    }

    void plannedSaveSize_respectsPendingSelectionSize() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = writeTestImage(dir, 200, 100);
        QVERIFY(!path.isEmpty());

        ImageDocument doc;
        QVERIFY(doc.load(path));

        QCOMPARE(doc.plannedSaveSize(true, 100, 100, QSize(50, 50)), QSize(50, 50));
    }

    void straighten_reRotatesBaseRatherThanCompounding() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = writeTestImage(dir, 100, 100);
        QVERIFY(!path.isEmpty());

        ImageDocument stepwise;
        QVERIFY(stepwise.load(path));
        stepwise.startStraighten();
        stepwise.applyStraighten(10.0, /*keepBase=*/true);
        stepwise.applyStraighten(5.0, /*keepBase=*/true);

        ImageDocument direct;
        QVERIFY(direct.load(path));
        direct.startStraighten();
        direct.applyStraighten(15.0, /*keepBase=*/true);

        QCOMPARE(stepwise.current(), direct.current());
    }

    void straighten_cancelLeavesCurrentImageUntouched() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = writeTestImage(dir, 100, 100);
        QVERIFY(!path.isEmpty());

        ImageDocument doc;
        QVERIFY(doc.load(path));
        doc.startStraighten();
        doc.applyStraighten(20.0, /*keepBase=*/true);
        const QImage rotated = doc.current();

        QVERIFY(doc.hasStraightenBase());
        doc.cancelStraighten();
        QVERIFY(!doc.hasStraightenBase());
        QCOMPARE(doc.current(), rotated);
    }

    void straighten_applyWithoutKeepBaseEndsSession() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = writeTestImage(dir, 100, 100);
        QVERIFY(!path.isEmpty());

        ImageDocument doc;
        QVERIFY(doc.load(path));
        doc.startStraighten();
        doc.applyStraighten(5.0, /*keepBase=*/false);
        QVERIFY(!doc.hasStraightenBase());
    }
};

QTEST_MAIN(ImageDocumentTest)
#include "ImageDocumentTest.moc"
