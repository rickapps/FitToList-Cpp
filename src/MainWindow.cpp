#include "MainWindow.h"

#include <QAction>
#include <QFileDialog>
#include <QFileInfo>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <cmath>

#include "CanvasWidget.h"
#include "FileNaming.h"

namespace {
QString imageFileDialogFilter() {
    QStringList patterns;
    for (const QString &ext : kImageExtensions) {
        patterns << "*" + ext;
    }
    return "Images (" + patterns.join(' ') + ")";
}

// Python's f"{angle:+.1f}" - always signed, one decimal place.
QString formatSignedAngle(double angleDegrees) {
    return QString("%1%2%3")
        .arg(angleDegrees < 0 ? "-" : "+")
        .arg(QString::number(std::abs(angleDegrees), 'f', 1))
        .arg(QChar(0x00B0));  // degree sign
}
}  // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    resize(1000, 700);

    canvas_ = new CanvasWidget(this);
    canvas_->setDocument(&document_);
    setCentralWidget(canvas_);

    statusMessage_ = new QLabel(this);
    statusBar()->addWidget(statusMessage_, 1);

    connect(canvas_, &CanvasWidget::interactionChanged, this, &MainWindow::updateMessage);
    connect(&document_, &ImageDocument::imageChanged, this, &MainWindow::updateMessage);
    connect(&document_, &ImageDocument::imageChanged, this, &MainWindow::updateWindowTitle);
    connect(&document_, &ImageDocument::imageLoaded, this, &MainWindow::updateMessage);
    connect(&document_, &ImageDocument::imageLoaded, this, &MainWindow::updateWindowTitle);

    buildMenusAndShortcuts();
    updateMessage();
    updateWindowTitle();
}

void MainWindow::buildMenusAndShortcuts() {
    QMenu *fileMenu = menuBar()->addMenu("&File");
    QAction *openAction = fileMenu->addAction("&Open Image...");
    connect(openAction, &QAction::triggered, this, &MainWindow::openImage);

    QMenu *editMenu = menuBar()->addMenu("&Edit");

    QAction *cropAction = editMenu->addAction("&Crop to Selection");
    cropAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));
    connect(cropAction, &QAction::triggered, canvas_, &CanvasWidget::cropToSelection);

    QAction *rotateRightAction = editMenu->addAction("Rotate &Right");
    rotateRightAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    connect(rotateRightAction, &QAction::triggered, &document_, &ImageDocument::rotateRight);

    QAction *rotateLeftAction = editMenu->addAction("Rotate &Left");
    rotateLeftAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
    connect(rotateLeftAction, &QAction::triggered, &document_, &ImageDocument::rotateLeft);

    QAction *straightenAction = editMenu->addAction("&Straighten");
    straightenAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_A));
    connect(straightenAction, &QAction::triggered, canvas_, &CanvasWidget::toggleStraighten);

    QAction *resetAction = editMenu->addAction("Rese&t");
    resetAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Z));
    connect(resetAction, &QAction::triggered, &document_, &ImageDocument::reset);

    // Escape isn't a menu item in the Python original either - just a
    // window-wide key binding to dismiss the Straighten tool.
    auto *cancelStraightenAction = new QAction(this);
    cancelStraightenAction->setShortcut(QKeySequence(Qt::Key_Escape));
    connect(cancelStraightenAction, &QAction::triggered, canvas_, &CanvasWidget::cancelStraighten);
    addAction(cancelStraightenAction);
}

void MainWindow::openImage() {
    const QString path = QFileDialog::getOpenFileName(this, "Open Image", QString(), imageFileDialogFilter());
    if (path.isEmpty()) {
        return;
    }
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
}

void MainWindow::updateMessage() {
    if (document_.current().isNull()) {
        statusMessage_->setText("Open an image to begin.");
        return;
    }
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
    statusMessage_->setText("Ready.");
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
