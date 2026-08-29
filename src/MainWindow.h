#pragma once

#include <QMainWindow>
#include <QSettings>
#include <QString>

#include "ImageDocument.h"

class CanvasWidget;
class ImageTreeWidget;
class QAction;
class QLabel;
class QPushButton;

// Top-level window: owns the one ImageDocument being edited, the source/
// processed folder paths and max-save-size settings (persisted via
// QSettings), and hosts an ImageTreeWidget (left) + CanvasWidget (right)
// split by the source/processed folder bar. Ports PhotoEditorApp's
// orchestration - folder selection, the tree-driven load/confirm-discard
// flow, and the save workflow - onto that split.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void selectFolders();
    void openSourceFolder();
    void openProcessedFolder();
    void editMaxSize();
    void onTreePathSelected(const QString &path);
    void saveCurrentAction();
    void processAndSave();
    void showUserGuide();
    void showAbout();
    void updateMessage();
    void updateWindowTitle();

private:
    void buildActions();
    void buildLayout();
    void buildMenus();
    void openWithDefaultApp(const QString &path, const QString &title, const QString &notFoundMessage);
    void openFolderInFileManager(const QString &path, const QString &title);
    bool isProcessedImage(const QString &path) const;
    bool hasUnsavedChanges() const;
    bool confirmDiscardChanges();
    void loadImage(const QString &path);
    void clearImageState();
    void persistConfig();
    void setMessage(const QString &text);
    void updateMaxSizeLabelText();
    bool saveCurrent(bool confirmOverwrite = true);

    QSettings settings_;
    ImageDocument document_;
    CanvasWidget *canvas_;
    ImageTreeWidget *tree_;
    QLabel *statusMessage_;
    QLabel *sourceFolderLabel_;
    QLabel *targetFolderLabel_;
    QPushButton *maxSizeButton_;

    // Toolbar actions, shared with their menu items (one QAction, two UI
    // surfaces) - the subset of actions the Python original also puts on its
    // toolbar. Everything else (Open Source Folder, Max Save Size..., Exit,
    // Reverse Image, Reset, Process & Save, the hidden Escape binding) stays
    // menu/shortcut-only, same as the original.
    QAction *selectFoldersAction_;
    QAction *openProcessedFolderAction_;
    QAction *rotateRightAction_;
    QAction *rotateLeftAction_;
    QAction *cropAction_;
    QAction *straightenAction_;
    QAction *saveAction_;

    QString currentPath_;
    QString sourceFolder_;
    QString targetFolder_;
    bool maxSizeEnabled_ = false;
    int maxWidth_ = 0;
    int maxHeight_ = 0;
    bool overwriteDeclined_ = false;
    QString statusMessageBase_ = "Select a photo from the list to begin.";
};
