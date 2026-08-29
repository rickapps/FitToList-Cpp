#include "DirectoryTreeWidget.h"

#include <QDir>
#include <QFileSystemModel>
#include <QHeaderView>

DirectoryTreeWidget::DirectoryTreeWidget(QWidget *parent) : QTreeView(parent) {
    model_ = new QFileSystemModel(this);
    model_->setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    // An empty root path/index gives the virtual "Computer" root: drive
    // letters on Windows, "/" transparently elsewhere - no platform-specific
    // code needed, unlike the Python original's _list_roots().
    model_->setRootPath(QString());
    setModel(model_);
    setRootIndex(model_->index(QString()));

    setHeaderHidden(true);
    for (int column = 1; column < model_->columnCount(); ++column) {
        hideColumn(column);
    }
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(selectionModel(), &QItemSelectionModel::currentChanged, this, &DirectoryTreeWidget::onCurrentChanged);
}

void DirectoryTreeWidget::setCurrentPath(const QString &path) {
    const QString target = (!path.isEmpty() && QDir(path).exists()) ? path : QDir::homePath();
    const QModelIndex index = model_->index(target);
    if (!index.isValid()) {
        return;
    }
    for (QModelIndex ancestor = index.parent(); ancestor.isValid(); ancestor = ancestor.parent()) {
        expand(ancestor);
    }
    setCurrentIndex(index);
    scrollTo(index);
}

QString DirectoryTreeWidget::currentPath() const {
    return model_->filePath(currentIndex());
}

void DirectoryTreeWidget::onCurrentChanged(const QModelIndex &current, const QModelIndex &) {
    if (!current.isValid()) {
        return;
    }
    emit pathChanged(model_->filePath(current));
}
