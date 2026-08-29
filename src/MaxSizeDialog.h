#pragma once

#include <QDialog>

class QCheckBox;
class QLineEdit;

// Modal dialog for setting the max width/height applied to images on save.
// Ports MaxSizeDialog.
class MaxSizeDialog : public QDialog {
    Q_OBJECT

public:
    MaxSizeDialog(QWidget *parent, bool enabled, int maxWidth, int maxHeight);

    // Valid only after exec() returns QDialog::Accepted.
    bool enabled() const { return resultEnabled_; }
    int maxWidth() const { return resultWidth_; }
    int maxHeight() const { return resultHeight_; }

private slots:
    void updateFieldState();
    void trySave();

private:
    QCheckBox *enabledCheck_;
    QLineEdit *widthEdit_;
    QLineEdit *heightEdit_;

    bool resultEnabled_ = false;
    int resultWidth_ = 0;
    int resultHeight_ = 0;
};
