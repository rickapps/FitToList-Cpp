#pragma once

#include <QImage>
#include <QObject>
#include <QRect>
#include <QSize>
#include <QString>

// Owns the image pixels and pure image-editing operations (crop, rotate,
// reverse, straighten, save-size planning). Holds no UI/interaction state:
// the crop rectangle and straighten drag angle are computed by CanvasWidget
// and passed in already in image-space.
class ImageDocument : public QObject {
    Q_OBJECT

public:
    explicit ImageDocument(QObject *parent = nullptr);

    bool load(const QString &path, QString *error = nullptr);

    QImage original() const { return original_; }
    QImage current() const { return current_; }
    bool isDirty() const { return dirty_; }

    // Crops current() to imageSpaceRect, clamped to image bounds. Returns
    // false (no-op) if there's no image loaded or the clamped rect is too
    // small to be a meaningful crop.
    bool crop(const QRect &imageSpaceRect);

    void rotateRight();
    void rotateLeft();
    void reverse();

    // Restores current() from original(), clearing dirty and straighten state.
    void reset();

    // Snapshots current() as the pristine base for a straighten session.
    void startStraighten();
    bool hasStraightenBase() const { return !straightenBase_.isNull(); }

    // Re-rotates the straighten base image by (accumulated total angle so
    // far + pendingAngleDegrees), replacing current(). If keepBase is true
    // the base snapshot and accumulated angle are kept for further fine
    // adjustment; otherwise they're cleared, ending the straighten session.
    void applyStraighten(double pendingAngleDegrees, bool keepBase);

    // Discards the straighten base/accumulated angle without touching current().
    void cancelStraighten();

    // Size current() would be saved at: pendingSelectionSize if valid (a
    // pending crop selection that Save would commit), else current()'s own
    // size — capped to fit within maxWidth x maxHeight only if that would
    // actually shrink it and maxSizeEnabled is set.
    QSize plannedSaveSize(bool maxSizeEnabled, int maxWidth, int maxHeight,
                          QSize pendingSelectionSize = QSize()) const;

    // Writes current() to path, converting to a non-alpha format first for
    // JPEG targets and resizing to targetSize if it differs from current()'s
    // size. Returns false and sets *error on failure.
    bool saveTo(const QString &path, QSize targetSize, QString *error = nullptr);

signals:
    void imageChanged();

private:
    QImage original_;
    QImage current_;
    bool dirty_ = false;

    QImage straightenBase_;
    double straightenTotalAngle_ = 0.0;
};
