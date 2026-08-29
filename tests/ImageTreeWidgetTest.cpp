#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "ImageTreeWidget.h"

namespace {
void touch(const QString &path) {
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();
}
}  // namespace

class ImageTreeWidgetTest : public QObject {
    Q_OBJECT

private slots:
    void refresh_populatesSourceAndProcessedNodes() {
        QTemporaryDir sourceDir;
        QTemporaryDir targetDir;
        QVERIFY(sourceDir.isValid() && targetDir.isValid());
        touch(sourceDir.path() + "/a.png");
        touch(sourceDir.path() + "/b.jpg");
        touch(sourceDir.path() + "/notes.txt");  // not an image, should be ignored
        touch(targetDir.path() + "/a_00.png");
        touch(targetDir.path() + "/a_01.png");

        ImageTreeWidget tree;
        tree.setFolders(sourceDir.path(), targetDir.path());
        tree.refresh(QString());

        QCOMPARE(tree.topLevelItemCount(), 2);
        QCOMPARE(tree.topLevelItem(0)->text(0), QString("a.png"));
        QCOMPARE(tree.topLevelItem(0)->childCount(), 2);
        QCOMPARE(tree.topLevelItem(0)->child(0)->text(0), QString("a_00.png"));
        QCOMPARE(tree.topLevelItem(0)->child(1)->text(0), QString("a_01.png"));
        QCOMPARE(tree.topLevelItem(1)->text(0), QString("b.jpg"));
        QCOMPARE(tree.topLevelItem(1)->childCount(), 0);
    }

    void refresh_withNoCurrentPathSelectsFirstUnprocessedImage() {
        QTemporaryDir sourceDir;
        QTemporaryDir targetDir;
        QVERIFY(sourceDir.isValid() && targetDir.isValid());
        touch(sourceDir.path() + "/a.png");
        touch(sourceDir.path() + "/b.jpg");
        touch(targetDir.path() + "/a_00.png");  // a.png already has processed output

        ImageTreeWidget tree;
        tree.setFolders(sourceDir.path(), targetDir.path());
        QSignalSpy spy(&tree, &ImageTreeWidget::pathSelected);
        tree.refresh(QString());

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).toString(), sourceDir.filePath("b.jpg"));
    }

    void refresh_fallsBackToFirstSourceWhenAllProcessed() {
        QTemporaryDir sourceDir;
        QTemporaryDir targetDir;
        QVERIFY(sourceDir.isValid() && targetDir.isValid());
        touch(sourceDir.path() + "/a.png");
        touch(targetDir.path() + "/a_00.png");

        ImageTreeWidget tree;
        tree.setFolders(sourceDir.path(), targetDir.path());
        QSignalSpy spy(&tree, &ImageTreeWidget::pathSelected);
        tree.refresh(QString());

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).toString(), sourceDir.filePath("a.png"));
    }

    void refresh_reselectsCurrentPathWithoutAutoSelecting() {
        QTemporaryDir sourceDir;
        QTemporaryDir targetDir;
        QVERIFY(sourceDir.isValid() && targetDir.isValid());
        touch(sourceDir.path() + "/a.png");
        touch(sourceDir.path() + "/b.jpg");

        ImageTreeWidget tree;
        tree.setFolders(sourceDir.path(), targetDir.path());
        QSignalSpy spy(&tree, &ImageTreeWidget::pathSelected);
        tree.refresh(sourceDir.filePath("b.jpg"));

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).toString(), sourceDir.filePath("b.jpg"));
    }

    void refreshProcessedChildren_rebuildsOnlyThatNode() {
        QTemporaryDir sourceDir;
        QTemporaryDir targetDir;
        QVERIFY(sourceDir.isValid() && targetDir.isValid());
        touch(sourceDir.path() + "/a.png");
        touch(sourceDir.path() + "/b.jpg");

        ImageTreeWidget tree;
        tree.setFolders(sourceDir.path(), targetDir.path());
        tree.refresh(QString());
        QCOMPARE(tree.topLevelItem(0)->childCount(), 0);

        touch(targetDir.path() + "/a_00.png");
        tree.refreshProcessedChildren(sourceDir.filePath("a.png"));

        QCOMPARE(tree.topLevelItemCount(), 2);  // untouched
        QCOMPARE(tree.topLevelItem(0)->childCount(), 1);
        QCOMPARE(tree.topLevelItem(0)->child(0)->text(0), QString("a_00.png"));
        QCOMPARE(tree.topLevelItem(1)->childCount(), 0);
    }

    void refreshProcessedChildren_noOpForAProcessedPath() {
        QTemporaryDir sourceDir;
        QTemporaryDir targetDir;
        QVERIFY(sourceDir.isValid() && targetDir.isValid());
        touch(sourceDir.path() + "/a.png");
        touch(targetDir.path() + "/a_00.png");

        ImageTreeWidget tree;
        tree.setFolders(sourceDir.path(), targetDir.path());
        tree.refresh(QString());

        // Passing a processed (child) path, not a source path, should do nothing.
        tree.refreshProcessedChildren(targetDir.filePath("a_00.png"));
        QCOMPARE(tree.topLevelItem(0)->childCount(), 1);
    }

    void selectPath_isNoOpWhenAlreadyCurrent() {
        QTemporaryDir sourceDir;
        QTemporaryDir targetDir;
        QVERIFY(sourceDir.isValid() && targetDir.isValid());
        touch(sourceDir.path() + "/a.png");

        ImageTreeWidget tree;
        tree.setFolders(sourceDir.path(), targetDir.path());
        tree.refresh(QString());  // auto-selects a.png

        QSignalSpy spy(&tree, &ImageTreeWidget::pathSelected);
        tree.selectPath(sourceDir.filePath("a.png"));
        QCOMPARE(spy.count(), 0);
    }

    void selectNextSourceAfter_advancesToFollowingSourceItem() {
        QTemporaryDir sourceDir;
        QTemporaryDir targetDir;
        QVERIFY(sourceDir.isValid() && targetDir.isValid());
        touch(sourceDir.path() + "/a.png");
        touch(sourceDir.path() + "/b.jpg");
        touch(sourceDir.path() + "/c.png");

        ImageTreeWidget tree;
        tree.setFolders(sourceDir.path(), targetDir.path());
        tree.refresh(sourceDir.filePath("a.png"));

        QSignalSpy spy(&tree, &ImageTreeWidget::pathSelected);
        tree.selectNextSourceAfter(sourceDir.filePath("a.png"));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).toString(), sourceDir.filePath("b.jpg"));

        // Advancing past the last source item is a no-op.
        tree.selectNextSourceAfter(sourceDir.filePath("c.png"));
        QCOMPARE(spy.count(), 0);
    }
};

QTEST_MAIN(ImageTreeWidgetTest)
#include "ImageTreeWidgetTest.moc"
