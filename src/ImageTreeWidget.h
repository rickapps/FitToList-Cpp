#pragma once

#include <QString>
#include <QTreeWidget>

// The main window's left-pane tree: one top-level item per source image in
// sourceFolder, with a child item for each processed output already saved
// for it in targetFolder (matched by the {root}_NN{ext} convention in
// FileNaming). Ports the file_tree pane's behavior from photo_editor.py -
// _populate_tree, _refresh_processed_children, _select_tree_node_for_current_image,
// _select_first_unprocessed_image, and _select_next_file - onto QTreeWidget,
// with each item's file path attached directly as Qt::UserRole data instead
// of the separate _tree_paths side-map the iid-based Tk API needed.
class ImageTreeWidget : public QTreeWidget {
    Q_OBJECT

public:
    explicit ImageTreeWidget(QWidget *parent = nullptr);

    void setFolders(const QString &sourceFolder, const QString &targetFolder);

    // Rebuilds the whole tree from disk. Re-selects currentPath's node if
    // it's still present; otherwise auto-selects the first source image with
    // no processed children yet (or the first source image overall if every
    // one already has output). Either selection fires pathSelected().
    void refresh(const QString &currentPath);

    // Rebuilds only sourcePath's processed children from disk, leaving the
    // rest of the tree and the current selection untouched. A no-op if
    // sourcePath isn't a top-level (source) item - e.g. it's itself a
    // processed file, whose own name a fresh save can't affect.
    void refreshProcessedChildren(const QString &sourcePath);

    // Selects and reveals path's node without rebuilding anything, or clears
    // the selection if path isn't found. A no-op (no signal) if path's node
    // is already current. Used to revert a cancelled tree-driven switch, and
    // to re-highlight a processed image after refreshProcessedChildren()
    // recreates its node.
    void selectPath(const QString &path);

    // Selects the source item immediately after currentPath's, if any -
    // advancing to the next photo after Process & Save. No-op if currentPath
    // isn't a source item, or is the last one.
    void selectNextSourceAfter(const QString &currentPath);

signals:
    // The current item changed to one with a resolved file path - covers
    // both a user click and any of the programmatic selections above.
    void pathSelected(const QString &path);

private slots:
    void onCurrentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

private:
    QTreeWidgetItem *findItemForPath(const QString &path) const;

    QString sourceFolder_;
    QString targetFolder_;
};
