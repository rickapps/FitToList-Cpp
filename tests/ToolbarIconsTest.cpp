#include <QImage>
#include <QtTest/QtTest>

#include "ToolbarIcons.h"

namespace {
bool hasVisibleContent(const QIcon &icon, int size) {
    const QImage image = icon.pixmap(size, size).toImage();
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(image.pixel(x, y)) > 0) {
                return true;
            }
        }
    }
    return false;
}
}  // namespace

class ToolbarIconsTest : public QObject {
    Q_OBJECT

private slots:
    void everyIconRendersAtRequestedSizeWithVisibleContent() {
        const int size = kToolbarIconSize;
        QVERIFY(hasVisibleContent(folderIcon(), size));
        QVERIFY(hasVisibleContent(openFolderIcon(), size));
        QVERIFY(hasVisibleContent(rotateRightIcon(), size));
        QVERIFY(hasVisibleContent(rotateLeftIcon(), size));
        QVERIFY(hasVisibleContent(cropIcon(), size));
        QVERIFY(hasVisibleContent(straightenIcon(), size));
        QVERIFY(hasVisibleContent(saveIcon(), size));
    }

    void rotateRightAndLeftIconsDiffer() {
        const int size = kToolbarIconSize;
        QVERIFY(rotateRightIcon().pixmap(size, size).toImage() != rotateLeftIcon().pixmap(size, size).toImage());
    }

    void respectsCustomSizeAndColor() {
        const int size = 48;
        const QIcon icon = saveIcon(size, Qt::red);
        const QImage image = icon.pixmap(size, size).toImage();
        QCOMPARE(image.size(), QSize(size, size));

        bool foundRed = false;
        for (int y = 0; y < image.height() && !foundRed; ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const QRgb pixel = image.pixel(x, y);
                if (qAlpha(pixel) > 0 && qRed(pixel) > 200 && qGreen(pixel) < 50 && qBlue(pixel) < 50) {
                    foundRed = true;
                    break;
                }
            }
        }
        QVERIFY(foundRed);
    }
};

QTEST_MAIN(ToolbarIconsTest)
#include "ToolbarIconsTest.moc"
