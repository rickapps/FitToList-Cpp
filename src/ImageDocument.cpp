#include "ImageDocument.h"

#include <QFileInfo>
#include <QPainter>
#include <QTransform>
#include <QtMath>
#include <algorithm>

namespace {

// Rotates source by angleDegrees, expanding the canvas to fit (like PIL's
// rotate(angle, expand=True)) and filling the exposed corners: transparent
// if the source has an alpha channel, opaque white otherwise - matching
// _straighten_fill_color's "'A' in mode" check in the Python original.
QImage rotateExpandingWithFill(const QImage &source, double angleDegrees) {
    const bool hasAlpha = source.hasAlphaChannel();
    const QImage argbSource = source.convertToFormat(QImage::Format_ARGB32);

    QTransform transform;
    transform.rotate(angleDegrees);
    const QImage rotated = argbSource.transformed(transform, Qt::SmoothTransformation);

    if (hasAlpha) {
        return rotated;
    }

    QImage result(rotated.size(), QImage::Format_ARGB32);
    result.fill(Qt::white);
    QPainter painter(&result);
    painter.drawImage(0, 0, rotated);
    painter.end();
    return result.convertToFormat(QImage::Format_RGB32);
}

}  // namespace

ImageDocument::ImageDocument(QObject *parent) : QObject(parent) {}

bool ImageDocument::load(const QString &path, QString *error) {
    QImage image(path);
    if (image.isNull()) {
        if (error) {
            *error = "Could not load image: " + path;
        }
        return false;
    }
    original_ = image;
    current_ = image;
    dirty_ = false;
    straightenBase_ = QImage();
    straightenTotalAngle_ = 0.0;
    emit imageLoaded();
    return true;
}

void ImageDocument::clear() {
    original_ = QImage();
    current_ = QImage();
    dirty_ = false;
    straightenBase_ = QImage();
    straightenTotalAngle_ = 0.0;
    emit imageLoaded();
}

bool ImageDocument::crop(const QRect &imageSpaceRect) {
    if (current_.isNull()) {
        return false;
    }
    const QRect bounds(0, 0, current_.width(), current_.height());
    const QRect clamped = imageSpaceRect.intersected(bounds);
    if (clamped.width() < 2 || clamped.height() < 2) {
        return false;
    }
    current_ = current_.copy(clamped);
    dirty_ = true;
    emit imageChanged();
    return true;
}

void ImageDocument::rotateRight() {
    if (current_.isNull()) {
        return;
    }
    current_ = current_.transformed(QTransform().rotate(90));
    dirty_ = true;
    emit imageChanged();
}

void ImageDocument::rotateLeft() {
    if (current_.isNull()) {
        return;
    }
    current_ = current_.transformed(QTransform().rotate(-90));
    dirty_ = true;
    emit imageChanged();
}

void ImageDocument::reverse() {
    if (current_.isNull()) {
        return;
    }
    current_ = current_.flipped(Qt::Horizontal);
    dirty_ = true;
    emit imageChanged();
}

void ImageDocument::reset() {
    if (original_.isNull()) {
        return;
    }
    current_ = original_;
    dirty_ = false;
    straightenBase_ = QImage();
    straightenTotalAngle_ = 0.0;
    emit imageChanged();
}

void ImageDocument::startStraighten() {
    if (current_.isNull()) {
        return;
    }
    straightenBase_ = current_;
    straightenTotalAngle_ = 0.0;
}

void ImageDocument::applyStraighten(double pendingAngleDegrees, bool keepBase) {
    if (straightenBase_.isNull()) {
        return;
    }
    if (pendingAngleDegrees != 0.0) {
        const double totalAngle = straightenTotalAngle_ + pendingAngleDegrees;
        current_ = rotateExpandingWithFill(straightenBase_, totalAngle);
        straightenTotalAngle_ = totalAngle;
        dirty_ = true;
        emit imageChanged();
    }
    if (!keepBase) {
        straightenBase_ = QImage();
        straightenTotalAngle_ = 0.0;
    }
}

void ImageDocument::cancelStraighten() {
    straightenBase_ = QImage();
    straightenTotalAngle_ = 0.0;
}

QSize ImageDocument::plannedSaveSize(bool maxSizeEnabled, int maxWidth, int maxHeight,
                                    QSize pendingSelectionSize) const {
    if (current_.isNull()) {
        return QSize();
    }
    QSize size = pendingSelectionSize.isValid() ? pendingSelectionSize : current_.size();
    if (maxSizeEnabled && maxWidth > 0 && maxHeight > 0) {
        const double scale = std::min({static_cast<double>(maxWidth) / size.width(),
                                        static_cast<double>(maxHeight) / size.height(), 1.0});
        if (scale < 1.0) {
            const int width = std::max(1, qRound(size.width() * scale));
            const int height = std::max(1, qRound(size.height() * scale));
            size = QSize(width, height);
        }
    }
    return size;
}

bool ImageDocument::saveTo(const QString &path, QSize targetSize, QString *error) {
    if (current_.isNull()) {
        if (error) {
            *error = "No image loaded to save.";
        }
        return false;
    }
    QImage imageToSave = current_;
    const QString suffix = QFileInfo(path).suffix().toLower();
    if ((suffix == "jpg" || suffix == "jpeg") && imageToSave.hasAlphaChannel()) {
        imageToSave = imageToSave.convertToFormat(QImage::Format_RGB32);
    }
    if (targetSize.isValid() && targetSize != imageToSave.size()) {
        imageToSave = imageToSave.scaled(targetSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    if (!imageToSave.save(path)) {
        if (error) {
            *error = "Could not save image: " + path;
        }
        return false;
    }
    dirty_ = false;
    emit imageChanged();
    return true;
}
