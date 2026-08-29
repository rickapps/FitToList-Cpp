#include "MaxSizeDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QRegularExpression>
#include <QVBoxLayout>

namespace {
bool isPositiveIntegerText(const QString &text) {
    static const QRegularExpression kDigitsOnly("^\\d+$");
    return kDigitsOnly.match(text).hasMatch();
}
}  // namespace

MaxSizeDialog::MaxSizeDialog(QWidget *parent, bool enabled, int maxWidth, int maxHeight) : QDialog(parent) {
    setWindowTitle("Max Save Size");

    auto *layout = new QVBoxLayout(this);
    layout->setSizeConstraint(QLayout::SetFixedSize);

    enabledCheck_ = new QCheckBox("Reduce images that exceed the maximum size when saving", this);
    enabledCheck_->setChecked(enabled);
    layout->addWidget(enabledCheck_);

    auto *fieldsLayout = new QGridLayout;
    widthEdit_ = new QLineEdit(maxWidth ? QString::number(maxWidth) : QString(), this);
    heightEdit_ = new QLineEdit(maxHeight ? QString::number(maxHeight) : QString(), this);
    fieldsLayout->addWidget(new QLabel("Max width (px):", this), 0, 0);
    fieldsLayout->addWidget(widthEdit_, 0, 1);
    fieldsLayout->addWidget(new QLabel("Max height (px):", this), 1, 0);
    fieldsLayout->addWidget(heightEdit_, 1, 1);
    layout->addLayout(fieldsLayout);

    connect(enabledCheck_, &QCheckBox::toggled, this, &MaxSizeDialog::updateFieldState);
    updateFieldState();

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &MaxSizeDialog::trySave);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void MaxSizeDialog::updateFieldState() {
    const bool enabled = enabledCheck_->isChecked();
    widthEdit_->setEnabled(enabled);
    heightEdit_->setEnabled(enabled);
}

void MaxSizeDialog::trySave() {
    const bool enabled = enabledCheck_->isChecked();
    const QString widthText = widthEdit_->text().trimmed();
    const QString heightText = heightEdit_->text().trimmed();

    if (enabled && (!isPositiveIntegerText(widthText) || !isPositiveIntegerText(heightText) ||
                    widthText.toInt() == 0 || heightText.toInt() == 0)) {
        QMessageBox::critical(this, "Max Save Size",
                               "Please enter positive whole numbers for max width and max height.");
        return;
    }

    resultEnabled_ = enabled;
    resultWidth_ = isPositiveIntegerText(widthText) ? widthText.toInt() : 0;
    resultHeight_ = isPositiveIntegerText(heightText) ? heightText.toInt() : 0;
    accept();
}
