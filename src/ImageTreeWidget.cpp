#include "ImageTreeWidget.h"

#include <QDir>
#include <QFileInfo>
#include <algorithm>

#include "FileNaming.h"

namespace {
constexpr int kPathRole = Qt::UserRole;

QString itemPath(const QTreeWidgetItem *item) {
    return item ? item->data(0, kPathRole).toString() : QString();
}
}  // namespace

ImageTreeWidget::ImageTreeWidget(QWidget *parent) : QTreeWidget(parent) {
    setHeaderHidden(true);
    setColumnCount(1);
    setSelectionMode(QAbstractItemView::SingleSelection);
    connect(this, &QTreeWidget::currentItemChanged, this, &ImageTreeWidget::onCurrentItemChanged);
}

void ImageTreeWidget::setFolders(const QString &sourceFolder, const QString &targetFolder) {
    sourceFolder_ = sourceFolder;
    targetFolder_ = targetFolder;
}

void ImageTreeWidget::refresh(const QString &currentPath) {
    clear();

    QDir sourceDir(sourceFolder_);
    QStringList sourceNames;
    if (sourceDir.exists()) {
        for (const QString &name : sourceDir.entryList(QDir::Files)) {
            if (isImageFile(name)) {
                sourceNames << name;
            }
        }
    }
    std::sort(sourceNames.begin(), sourceNames.end());

    QDir targetDir(targetFolder_);
    const QStringList processedNames = targetDir.exists() ? targetDir.entryList(QDir::Files) : QStringList();

    for (const QString &name : sourceNames) {
        auto *sourceItem = new QTreeWidgetItem(this, {name});
        sourceItem->setData(0, kPathRole, sourceDir.filePath(name));
        for (const QString &processedName : matchingProcessedNames(name, processedNames)) {
            auto *childItem = new QTreeWidgetItem(sourceItem, {processedName});
            childItem->setData(0, kPathRole, targetDir.filePath(processedName));
        }
    }

    QTreeWidgetItem *itemToSelect = currentPath.isEmpty() ? nullptr : findItemForPath(currentPath);
    if (!itemToSelect) {
        for (int i = 0; i < topLevelItemCount(); ++i) {
            if (topLevelItem(i)->childCount() == 0) {
                itemToSelect = topLevelItem(i);
                break;
            }
        }
        if (!itemToSelect && topLevelItemCount() > 0) {
            itemToSelect = topLevelItem(0);
        }
    }
    if (itemToSelect) {
        setCurrentItem(itemToSelect);
        scrollToItem(itemToSelect);
    }
}

void ImageTreeWidget::refreshProcessedChildren(const QString &sourcePath) {
    QTreeWidgetItem *sourceItem = nullptr;
    for (int i = 0; i < topLevelItemCount(); ++i) {
        if (itemPath(topLevelItem(i)) == sourcePath) {
            sourceItem = topLevelItem(i);
            break;
        }
    }
    if (!sourceItem) {
        return;
    }

    qDeleteAll(sourceItem->takeChildren());

    const QString sourceName = QFileInfo(sourcePath).fileName();
    QDir targetDir(targetFolder_);
    const QStringList processedNames = targetDir.exists() ? targetDir.entryList(QDir::Files) : QStringList();
    for (const QString &processedName : matchingProcessedNames(sourceName, processedNames)) {
        auto *childItem = new QTreeWidgetItem(sourceItem, {processedName});
        childItem->setData(0, kPathRole, targetDir.filePath(processedName));
    }
    if (sourceItem->childCount() > 0) {
        sourceItem->setExpanded(true);
    }
}

void ImageTreeWidget::selectPath(const QString &path) {
    QTreeWidgetItem *item = path.isEmpty() ? nullptr : findItemForPath(path);
    if (item) {
        setCurrentItem(item);
        scrollToItem(item);
    } else {
        clearSelection();
        setCurrentItem(nullptr);
    }
}

void ImageTreeWidget::selectNextSourceAfter(const QString &currentPath) {
    const QString currentName = QFileInfo(currentPath).fileName();
    int nextIndex = 0;
    bool found = false;
    for (int i = 0; i < topLevelItemCount(); ++i) {
        if (topLevelItem(i)->text(0) == currentName) {
            nextIndex = i + 1;
            found = true;
            break;
        }
    }
    if (!found) {
        nextIndex = 0;
    }
    if (nextIndex >= topLevelItemCount()) {
        return;
    }
    QTreeWidgetItem *item = topLevelItem(nextIndex);
    setCurrentItem(item);
    scrollToItem(item);
}

QTreeWidgetItem *ImageTreeWidget::findItemForPath(const QString &path) const {
    for (int i = 0; i < topLevelItemCount(); ++i) {
        QTreeWidgetItem *top = topLevelItem(i);
        if (itemPath(top) == path) {
            return top;
        }
        for (int j = 0; j < top->childCount(); ++j) {
            if (itemPath(top->child(j)) == path) {
                return top->child(j);
            }
        }
    }
    return nullptr;
}

void ImageTreeWidget::onCurrentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *) {
    const QString path = itemPath(current);
    if (!path.isEmpty()) {
        emit pathSelected(path);
    }
}
