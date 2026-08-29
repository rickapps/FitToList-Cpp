#pragma once

#include <QTreeView>

class QFileSystemModel;

// A whole-filesystem directory browser (drives, or a single root on
// non-Windows platforms shown transparently by QFileSystemModel's virtual
// "" root), used by FolderSelectionDialog to pick the source/processed
// folders. Ports FolderTreeFrame from the Python original, but almost all of
// that class's code (lazy child population, placeholder nodes, expand-to-path)
// existed only because Tkinter's Treeview has no built-in filesystem model -
// QFileSystemModel provides all of it natively.
class DirectoryTreeWidget : public QTreeView {
    Q_OBJECT

public:
    explicit DirectoryTreeWidget(QWidget *parent = nullptr);

    // Selects and reveals path, expanding its ancestors so it's visible.
    // Falls back to the home directory if path is empty or doesn't exist.
    void setCurrentPath(const QString &path);

    QString currentPath() const;

signals:
    // The highlighted folder changed - mirrors FolderTreeFrame's path_var.
    void pathChanged(const QString &path);

private slots:
    void onCurrentChanged(const QModelIndex &current, const QModelIndex &previous);

private:
    QFileSystemModel *model_;
};
