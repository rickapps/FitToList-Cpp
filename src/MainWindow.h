#pragma once

#include <QMainWindow>
#include <QString>

#include "ImageDocument.h"

class CanvasWidget;
class QLabel;

// Top-level window: owns the one ImageDocument being edited and hosts a
// CanvasWidget as its central widget. Menu/shortcuts here cover the actions
// CanvasWidget and ImageDocument already expose (open, crop, rotate,
// straighten, reset); the file tree, folder dialogs, and save workflow that
// select *which* image is loaded and *where* it's written come in a later
// step, so "Open Image..." is a plain file dialog for now.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void openImage();
    void updateMessage();
    void updateWindowTitle();

private:
    void buildMenusAndShortcuts();

    ImageDocument document_;
    CanvasWidget *canvas_;
    QLabel *statusMessage_;
    QString currentPath_;
};
