#pragma once

#include <QMainWindow>
#include <QSettings>
#include <QString>

#include "ImageDocument.h"

class CanvasWidget;
class ImageTreeWidget;
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
    void updateMessage();
    void updateWindowTitle();

private:
    void buildMenusAndShortcuts();
    void buildLayout();
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

    QString currentPath_;
    QString sourceFolder_;
    QString targetFolder_;
    bool maxSizeEnabled_ = false;
    int maxWidth_ = 0;
    int maxHeight_ = 0;
    bool overwriteDeclined_ = false;
    QString statusMessageBase_ = "Select a photo from the list to begin.";
};
