#include "MainWindow.h"

#include <QAction>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>
#include <cmath>

#include "CanvasWidget.h"
#include "Config.h"
#include "FileNaming.h"
#include "FolderSelectionDialog.h"
#include "ImageTreeWidget.h"
#include "MaxSizeDialog.h"

namespace {
// Python's f"{angle:+.1f}" - always signed, one decimal place.
QString formatSignedAngle(double angleDegrees) {
    return QString("%1%2%3")
        .arg(angleDegrees < 0 ? "-" : "+")
        .arg(QString::number(std::abs(angleDegrees), 'f', 1))
        .arg(QChar(0x00B0));  // degree sign
}
}  // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    // settings_ picks up whatever QCoreApplication::setOrganizationName()/
    // setApplicationName() (and, in tests, QSettings::setPath()) are in
    // effect - see main.cpp for the app, MainWindowTest::initTestCase() for
    // tests, which redirect it to a throwaway location.
    resize(1000, 700);

    const AppConfig config = loadConfig(settings_);
    sourceFolder_ = config.sourceFolder.isEmpty() ? QDir::currentPath() : config.sourceFolder;
    targetFolder_ = config.targetFolder;
    maxSizeEnabled_ = config.maxSizeEnabled;
    maxWidth_ = config.maxWidth;
    maxHeight_ = config.maxHeight;

    buildLayout();
    buildMenusAndShortcuts();

    connect(canvas_, &CanvasWidget::interactionChanged, this, &MainWindow::updateMessage);
    connect(canvas_, &CanvasWidget::processAndSaveRequested, this, &MainWindow::processAndSave);
    connect(&document_, &ImageDocument::imageChanged, this, &MainWindow::updateMessage);
    connect(&document_, &ImageDocument::imageChanged, this, &MainWindow::updateWindowTitle);
    connect(&document_, &ImageDocument::imageLoaded, this, &MainWindow::updateMessage);
    connect(&document_, &ImageDocument::imageLoaded, this, &MainWindow::updateWindowTitle);
    connect(tree_, &ImageTreeWidget::pathSelected, this, &MainWindow::onTreePathSelected);

    tree_->setFolders(sourceFolder_, targetFolder_);
    tree_->refresh(currentPath_);

    updateMessage();
    updateWindowTitle();
}

void MainWindow::buildLayout() {
    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *folderBar = new QWidget(central);
    auto *folderForm = new QFormLayout(folderBar);
    folderForm->setContentsMargins(6, 4, 6, 4);

    sourceFolderLabel_ = new QLabel(sourceFolder_, folderBar);
    targetFolderLabel_ = new QLabel(targetFolder_.isEmpty() ? "Processed folder not set" : targetFolder_, folderBar);
    maxSizeButton_ = new QPushButton(folderBar);
    maxSizeButton_->setFlat(true);
    maxSizeButton_->setCursor(Qt::PointingHandCursor);
    maxSizeButton_->setToolTip("Click to change the max save size");
    connect(maxSizeButton_, &QPushButton::clicked, this, &MainWindow::editMaxSize);
    updateMaxSizeLabelText();

    folderForm->addRow("Source:", sourceFolderLabel_);
    folderForm->addRow("Processed:", targetFolderLabel_);
    folderForm->addRow("Max Save Size:", maxSizeButton_);
    layout->addWidget(folderBar);

    auto *splitter = new QSplitter(Qt::Horizontal, central);
    tree_ = new ImageTreeWidget(splitter);
    canvas_ = new CanvasWidget(splitter);
    canvas_->setDocument(&document_);
    splitter->addWidget(tree_);
    splitter->addWidget(canvas_);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({250, 750});
    layout->addWidget(splitter, 1);

    setCentralWidget(central);

    statusMessage_ = new QLabel(this);
    statusBar()->addWidget(statusMessage_, 1);
}

void MainWindow::buildMenusAndShortcuts() {
    QMenu *fileMenu = menuBar()->addMenu("&File");

    QAction *selectFoldersAction = fileMenu->addAction("&Select Folders...");
    selectFoldersAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_O));
    connect(selectFoldersAction, &QAction::triggered, this, &MainWindow::selectFolders);

    connect(fileMenu->addAction("Open Source Folder"), &QAction::triggered, this, &MainWindow::openSourceFolder);
    connect(fileMenu->addAction("Open Processed Folder"), &QAction::triggered, this,
            &MainWindow::openProcessedFolder);
    connect(fileMenu->addAction("&Max Save Size..."), &QAction::triggered, this, &MainWindow::editMaxSize);

    fileMenu->addSeparator();
    QAction *saveAction = fileMenu->addAction("&Save");
    saveAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_S));
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveCurrentAction);

    fileMenu->addSeparator();
    QAction *exitAction = fileMenu->addAction("E&xit");
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);

    QMenu *actionsMenu = menuBar()->addMenu("&Actions");

    QAction *cropAction = actionsMenu->addAction("&Crop to Selection");
    cropAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));
    connect(cropAction, &QAction::triggered, canvas_, &CanvasWidget::cropToSelection);

    QAction *straightenAction = actionsMenu->addAction("&Straighten");
    straightenAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_A));
    connect(straightenAction, &QAction::triggered, canvas_, &CanvasWidget::toggleStraighten);

    QAction *rotateRightAction = actionsMenu->addAction("Rotate &Right");
    rotateRightAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    connect(rotateRightAction, &QAction::triggered, &document_, &ImageDocument::rotateRight);

    QAction *rotateLeftAction = actionsMenu->addAction("Rotate &Left");
    rotateLeftAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
    connect(rotateLeftAction, &QAction::triggered, &document_, &ImageDocument::rotateLeft);

    connect(actionsMenu->addAction("Re&verse Image"), &QAction::triggered, &document_, &ImageDocument::reverse);

    QAction *resetAction = actionsMenu->addAction("Rese&t");
    resetAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Z));
    connect(resetAction, &QAction::triggered, &document_, &ImageDocument::reset);

    actionsMenu->addSeparator();
    QAction *processAndSaveAction = actionsMenu->addAction("&Process && Save");
    processAndSaveAction->setStatusTip("Double-click inside the selection does the same thing");
    connect(processAndSaveAction, &QAction::triggered, this, &MainWindow::processAndSave);

    // Escape isn't a menu item in the Python original either - just a
    // window-wide key binding to dismiss the Straighten tool.
    auto *cancelStraightenAction = new QAction(this);
    cancelStraightenAction->setShortcut(QKeySequence(Qt::Key_Escape));
    connect(cancelStraightenAction, &QAction::triggered, canvas_, &CanvasWidget::cancelStraighten);
    addAction(cancelStraightenAction);
}

void MainWindow::selectFolders() {
    FolderSelectionDialog dialog(this, sourceFolder_, targetFolder_);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const QString newSource = dialog.selectedSource();
    const QString newTarget = dialog.selectedTarget();
    const bool sourceChanged =
        QFileInfo(newSource).absoluteFilePath() != QFileInfo(sourceFolder_).absoluteFilePath();
    const bool targetChanged =
        targetFolder_.isEmpty() || QFileInfo(newTarget).absoluteFilePath() != QFileInfo(targetFolder_).absoluteFilePath();
    if (sourceChanged && !confirmDiscardChanges()) {
        return;
    }
    sourceFolder_ = newSource;
    targetFolder_ = newTarget;
    sourceFolderLabel_->setText(sourceFolder_);
    targetFolderLabel_->setText(targetFolder_);
    if (sourceChanged) {
        clearImageState();
    }
    if (sourceChanged || targetChanged) {
        tree_->setFolders(sourceFolder_, targetFolder_);
        tree_->refresh(currentPath_);
    }
    persistConfig();
}

void MainWindow::openSourceFolder() { openFolderInFileManager(sourceFolder_, "Open Source Folder"); }

void MainWindow::openProcessedFolder() {
    if (targetFolder_.isEmpty()) {
        QMessageBox::warning(this, "Open Processed Folder", "Please set a processed folder first.");
        return;
    }
    openFolderInFileManager(targetFolder_, "Open Processed Folder");
}

void MainWindow::editMaxSize() {
    MaxSizeDialog dialog(this, maxSizeEnabled_, maxWidth_, maxHeight_);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    maxSizeEnabled_ = dialog.enabled();
    maxWidth_ = dialog.maxWidth();
    maxHeight_ = dialog.maxHeight();
    persistConfig();
    updateMaxSizeLabelText();
    updateMessage();
}

void MainWindow::onTreePathSelected(const QString &path) {
    if (path == currentPath_) {
        return;
    }
    if (!confirmDiscardChanges()) {
        tree_->selectPath(currentPath_);
        return;
    }
    loadImage(path);
}

void MainWindow::saveCurrentAction() { saveCurrent(true); }

bool MainWindow::saveCurrent(bool confirmOverwrite) {
    if (document_.current().isNull()) {
        QMessageBox::information(this, "Save", "No image loaded to save.");
        return false;
    }
    if (targetFolder_.isEmpty()) {
        QMessageBox::warning(this, "Save", "Please set a processed folder first.");
        return false;
    }
    const QString filename = QFileInfo(currentPath_).fileName();
    QString path;
    if (isProcessedImage(currentPath_)) {
        if (confirmOverwrite && document_.isDirty()) {
            const auto reply = QMessageBox::question(this, "Save", QString("Overwrite %1 with your changes?").arg(filename),
                                                       QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes) {
                overwriteDeclined_ = true;
                return false;
            }
        }
        path = currentPath_;
    } else {
        const QFileInfo info(filename);
        const QString root = info.completeBaseName();
        const QString ext = info.suffix().isEmpty() ? QString() : "." + info.suffix();
        const int suffix = nextSuffix(targetFolder_, root, ext);
        path = QDir(targetFolder_).filePath(QString("%1_%2%3").arg(root).arg(suffix, 2, 10, QChar('0')).arg(ext));
    }
    const QSize saveSize =
        document_.plannedSaveSize(maxSizeEnabled_, maxWidth_, maxHeight_, canvas_->selectionImageSize());
    QString error;
    if (!document_.saveTo(path, saveSize, &error)) {
        QMessageBox::critical(this, "Error", error);
        return false;
    }
    overwriteDeclined_ = false;
    tree_->refreshProcessedChildren(currentPath_);
    setMessage(QString("%1 saved to Processed Folder").arg(QFileInfo(path).fileName()));
    return true;
}

void MainWindow::processAndSave() {
    if (document_.current().isNull()) {
        return;
    }
    if (canvas_->hasSelection() && !canvas_->cropToSelection()) {
        return;
    }
    const bool wasProcessed = isProcessedImage(currentPath_);
    if (!saveCurrent(true)) {
        return;
    }
    if (wasProcessed) {
        tree_->selectPath(currentPath_);
    } else {
        tree_->selectNextSourceAfter(currentPath_);
    }
}

void MainWindow::updateMessage() {
    if (canvas_->isStraightenActive()) {
        statusMessage_->setText(QString("Straighten: %1   |   Drag either end of the line to straighten, "
                                        "Esc or click elsewhere to finish.")
                                     .arg(formatSignedAngle(canvas_->straightenAngle())));
        return;
    }
    const QSize selection = canvas_->selectionImageSize();
    if (selection.isValid() && selection.width() > 0) {
        statusMessage_->setText(QString("Selection: %1 x %2   |   Ctrl-E to crop OR double click to process "
                                        "and save.")
                                     .arg(selection.width())
                                     .arg(selection.height()));
        return;
    }
    statusMessage_->setText(statusMessageBase_);
}

void MainWindow::updateWindowTitle() {
    if (currentPath_.isEmpty() || document_.current().isNull()) {
        setWindowTitle("FitToList");
        setWindowModified(false);
        return;
    }
    const QImage &image = document_.current();
    setWindowTitle(QString("%1 (%2 x %3)[*] %4 FitToList")
                       .arg(QFileInfo(currentPath_).fileName())
                       .arg(image.width())
                       .arg(image.height())
                       .arg(QChar(0x2014)));  // em dash
    setWindowModified(document_.isDirty());
}

void MainWindow::openFolderInFileManager(const QString &path, const QString &title) {
    if (!QFileInfo::exists(path)) {
        QMessageBox::critical(this, title, QString("Folder not found:\n%1").arg(path));
        return;
    }
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(path))) {
        QMessageBox::critical(this, title, QString("Could not open:\n%1").arg(path));
    }
}

bool MainWindow::isProcessedImage(const QString &path) const {
    return !targetFolder_.isEmpty() && QFileInfo(path).absolutePath() == QFileInfo(targetFolder_).absoluteFilePath();
}

bool MainWindow::hasUnsavedChanges() const {
    return !document_.current().isNull() &&
           (document_.isDirty() || canvas_->hasSelection() || canvas_->straightenAngle() != 0.0);
}

bool MainWindow::confirmDiscardChanges() {
    if (!hasUnsavedChanges()) {
        return true;
    }
    if (overwriteDeclined_) {
        return true;
    }
    const QString filename = QFileInfo(currentPath_).fileName();
    const QString prompt =
        isProcessedImage(currentPath_)
            ? QString("You have unsaved changes. Overwrite %1 with your changes before continuing?").arg(filename)
            : QString("%1 has unsaved changes. Save before continuing?").arg(filename);
    const auto reply = QMessageBox::question(this, "Unsaved Changes", prompt,
                                              QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                                              QMessageBox::Cancel);
    if (reply == QMessageBox::Cancel) {
        return false;
    }
    if (reply == QMessageBox::Yes) {
        if (canvas_->isStraightenActive()) {
            canvas_->finalizeStraighten();
        }
        if (canvas_->hasSelection() && !canvas_->cropToSelection()) {
            return false;
        }
        // Already confirmed above (with overwrite-specific wording if applicable) - don't ask again.
        return saveCurrent(false);
    }
    return true;
}

void MainWindow::loadImage(const QString &path) {
    // Set before load(): it emits imageLoaded() synchronously, which drives
    // updateWindowTitle() off currentPath_, so it must already be current.
    const QString previousPath = currentPath_;
    currentPath_ = path;
    QString error;
    if (!document_.load(path, &error)) {
        currentPath_ = previousPath;
        QMessageBox::critical(this, "Error", error);
        return;
    }
    overwriteDeclined_ = false;
    setMessage("Drag on the image to select a crop area.");
}

void MainWindow::clearImageState() {
    document_.clear();
    currentPath_.clear();
    overwriteDeclined_ = false;
    setMessage("Select a photo from the list to begin.");
}

void MainWindow::persistConfig() {
    AppConfig config;
    config.sourceFolder = sourceFolder_;
    config.targetFolder = targetFolder_;
    config.maxSizeEnabled = maxSizeEnabled_;
    config.maxWidth = maxWidth_;
    config.maxHeight = maxHeight_;
    saveConfig(config, settings_);
}

void MainWindow::setMessage(const QString &text) {
    statusMessageBase_ = text;
    updateMessage();
}

void MainWindow::updateMaxSizeLabelText() {
    if (maxSizeEnabled_ && maxWidth_ && maxHeight_) {
        maxSizeButton_->setText(QString("%1 x %2").arg(maxWidth_).arg(maxHeight_));
    } else {
        maxSizeButton_->setText("not set");
    }
}
