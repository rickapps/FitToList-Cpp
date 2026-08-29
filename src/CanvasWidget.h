#pragma once

#include <QLineF>
#include <QPointF>
#include <QRectF>
#include <QWidget>
#include <optional>

#include "ImageDocument.h"

class QPainter;

// Displays an ImageDocument's current image, fit-to-widget and centered, and
// drives two mutually exclusive mouse-driven overlays on top of it: a
// draggable/resizable crop-selection rectangle, and the Straighten tool's
// guide line. Owns only interaction state (drag mode, hover cursor, the
// selection rectangle and pending straighten angle) - all pixel editing is
// delegated to the ImageDocument it's pointed at.
class CanvasWidget : public QWidget {
    Q_OBJECT

public:
    explicit CanvasWidget(QWidget *parent = nullptr);

    // document is not owned; pass nullptr to detach.
    void setDocument(ImageDocument *document);

    // ---------- Crop selection ----------
    bool hasSelection() const { return selectionBox_.has_value(); }
    void clearSelection();

    // Size (in image pixels) the current selection would crop to, or an
    // invalid QSize if there is no selection or it's currently degenerate.
    QSize selectionImageSize() const;

    // Commits the current selection via the document, clearing it on success.
    // Returns false (no-op) if there's no selection or it's too small.
    bool cropToSelection();

    // ---------- Straighten ----------
    bool isStraightenActive() const { return straightenActive_; }
    double straightenAngle() const { return straightenAngle_; }
    void toggleStraighten();
    void startStraighten();
    void cancelStraighten();

    // Commits the pending drag angle (if any) and ends the session, unlike a
    // normal drag-release which keeps it open for fine-tuning. Used when the
    // straighten tool is still open at a point that requires it to be fully
    // resolved one way or the other, such as switching to a different image.
    void finalizeStraighten();

signals:
    // Selection box or pending straighten angle changed - status/message text
    // driven by this canvas should refresh.
    void interactionChanged();

    // Double-click landed inside the crop selection.
    void processAndSaveRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    enum class DragMode { Idle, New, Move, ResizeLeft, ResizeRight, ResizeTop, ResizeBottom };
    enum class SelectionHit { NoHit, Left, Right, Top, Bottom, Inside };
    enum class StraightenHandle { NoHandle, Left, Right };

    struct AxisBoundsF {
        double left, right, top, bottom;
        bool valid;
    };

    void updateDisplayGeometry();
    QPointF clampToImage(const QPointF &pos) const;
    SelectionHit hitTestSelection(const QPointF &pos) const;
    void moveSelection(const QPointF &pos);
    void resizeSelection(DragMode edge, const QPointF &pos);
    void growSelectionToMinSize();
    AxisBoundsF selectionImageBoundsF() const;
    QRect selectionImageCropRect() const;
    void updateHoverCursor(const QPointF &pos);

    std::optional<QLineF> straightenLinePoints() const;
    StraightenHandle straightenHitTest(const QPointF &pos) const;
    void updateStraightenAngle(const QPointF &pos);

    void drawSelection(QPainter &painter) const;
    void drawStraighten(QPainter &painter) const;

    ImageDocument *document_ = nullptr;

    double displayScale_ = 1.0;
    QPointF displayOffset_{0.0, 0.0};

    std::optional<QRectF> selectionBox_;
    QPointF selectionStart_;
    DragMode dragMode_ = DragMode::Idle;
    QPointF dragOffset_;
    QSizeF selectionSizeAtDragStart_;

    bool straightenActive_ = false;
    double straightenAngle_ = 0.0;
    StraightenHandle straightenDragHandle_ = StraightenHandle::NoHandle;
};
