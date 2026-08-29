#include <QFile>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "FileNaming.h"

namespace {
void createFile(const QString &path) {
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();
}
}  // namespace

class FileNamingTest : public QObject {
    Q_OBJECT

private slots:
    void nextSuffix_emptyFolder() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QCOMPARE(nextSuffix(dir.path(), "photo", ".jpg"), 0);
    }

    void nextSuffix_withGaps() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        createFile(dir.path() + "/photo_00.jpg");
        createFile(dir.path() + "/photo_03.jpg");
        createFile(dir.path() + "/photo_01.jpg");
        QCOMPARE(nextSuffix(dir.path(), "photo", ".jpg"), 4);
    }

    void nextSuffix_caseInsensitiveExtensionAndIgnoresOtherRoots() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        createFile(dir.path() + "/photo_00.JPG");
        createFile(dir.path() + "/other_05.jpg");
        QCOMPARE(nextSuffix(dir.path(), "photo", ".jpg"), 1);
    }

    void matchingProcessedNames_sortsBySuffixAndIgnoresOtherRoots() {
        const QStringList processed = {"photo_02.jpg", "photo_00.jpg", "photo_01.jpg", "other_00.jpg"};
        const QStringList matches = matchingProcessedNames("photo.jpg", processed);
        QCOMPARE(matches, QStringList({"photo_00.jpg", "photo_01.jpg", "photo_02.jpg"}));
    }

    void isImageFile_recognizesKnownExtensionsCaseInsensitively() {
        QVERIFY(isImageFile("photo.PNG"));
        QVERIFY(isImageFile("photo.jpg"));
        QVERIFY(!isImageFile("notes.txt"));
        QVERIFY(!isImageFile("noextension"));
    }
};

QTEST_MAIN(FileNamingTest)
#include "FileNamingTest.moc"
