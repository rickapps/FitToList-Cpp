#include "CanvasWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QtMath>
#include <algorithm>
#include <cmath>

namespace {
constexpr int kSelectionBorderMargin = 6;   // canvas px
constexpr int kSelectionMinSize = 32;       // canvas px, so edges stay grabbable regardless of zoom
constexpr int kSelectionClickThreshold = 4;  // canvas px
constexpr double kStraightenMaxAngle = 30.0;
constexpr int kStraightenHandleMargin = 8;

double clampAxis(double value, double lo, double hi) {
    return std::max(lo, std::min(hi, value));
}
}  // namespace

CanvasWidget::CanvasWidget(QWidget *parent) : QWidget(parent) {
    setMouseTracking(true);
}

void CanvasWidget::setDocument(ImageDocument *document) {
    if (document_) {
        disconnect(document_, nullptr, this, nullptr);
    }
    document_ = document;
    if (document_) {
        connect(document_, &ImageDocument::imageChanged, this, [this] {
            clearSelection();
            update();
        });
        connect(document_, &ImageDocument::imageLoaded, this, [this] {
            cancelStraighten();
            clearSelection();
            update();
        });
    }
    update();
}

// ---------- Painting ----------

void CanvasWidget::updateDisplayGeometry() {
    if (!document_ || document_->current().isNull()) {
        displayScale_ = 1.0;
        displayOffset_ = QPointF(0.0, 0.0);
        return;
    }
    const int canvasW = width();
    const int canvasH = height();
    if (canvasW <= 1 || canvasH <= 1) {
        return;
    }
    const QSize imgSize = document_->current().size();
    const double scale = std::min(static_cast<double>(canvasW) / imgSize.width(),
                                   static_cast<double>(canvasH) / imgSize.height());
    const double dispW = std::max(1.0, imgSize.width() * scale);
    const double dispH = std::max(1.0, imgSize.height() * scale);
    displayScale_ = scale;
    displayOffset_ = QPointF((canvasW - dispW) / 2.0, (canvasH - dispH) / 2.0);
}

void CanvasWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor("#333333"));
    if (!document_ || document_->current().isNull()) {
        return;
    }
    updateDisplayGeometry();
    const QImage &image = document_->current();
    const QRectF targetRect(displayOffset_,
                             QSizeF(image.width() * displayScale_, image.height() * displayScale_));
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawImage(targetRect, image);
    drawSelection(painter);
    drawStraighten(painter);
}

void CanvasWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    updateDisplayGeometry();
    update();
}

void CanvasWidget::drawSelection(QPainter &painter) const {
    if (!selectionBox_.has_value()) {
        return;
    }
    painter.setPen(QPen(Qt::red, 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(selectionBox_->normalized());
}

void CanvasWidget::drawStraighten(QPainter &painter) const {
    const std::optional<QLineF> line = straightenLinePoints();
    if (!line.has_value()) {
        return;
    }
    painter.setPen(QPen(Qt::yellow, 2));
    painter.drawLine(*line);
    constexpr double r = 5.0;
    painter.setBrush(Qt::yellow);
    painter.setPen(QPen(Qt::black, 1));
    for (const QPointF &p : {line->p1(), line->p2()}) {
        painter.drawEllipse(p, r, r);
    }
}

// ---------- Crop selection ----------

void CanvasWidget::clearSelection() {
    const bool hadSelection = selectionBox_.has_value() || dragMode_ != DragMode::Idle;
    selectionBox_.reset();
    dragMode_ = DragMode::Idle;
    update();
    if (hadSelection) {
        emit interactionChanged();
    }
}

CanvasWidget::AxisBoundsF CanvasWidget::selectionImageBoundsF() const {
    if (!document_ || document_->current().isNull() || !selectionBox_.has_value()) {
        return {0, 0, 0, 0, false};
    }
    const QRectF norm = selectionBox_->normalized();
    const QSize imgSize = document_->current().size();
    return {
        clampAxis((norm.left() - displayOffset_.x()) / displayScale_, 0.0, imgSize.width()),
        clampAxis((norm.right() - displayOffset_.x()) / displayScale_, 0.0, imgSize.width()),
        clampAxis((norm.top() - displayOffset_.y()) / displayScale_, 0.0, imgSize.height()),
        clampAxis((norm.bottom() - displayOffset_.y()) / displayScale_, 0.0, imgSize.height()),
        true,
    };
}

QSize CanvasWidget::selectionImageSize() const {
    const AxisBoundsF b = selectionImageBoundsF();
    if (!b.valid) {
        return QSize();
    }
    const int w = qRound(b.right - b.left);
    const int h = qRound(b.bottom - b.top);
    return (w > 0 && h > 0) ? QSize(w, h) : QSize();
}

QRect CanvasWidget::selectionImageCropRect() const {
    const AxisBoundsF b = selectionImageBoundsF();
    if (!b.valid) {
        return QRect();
    }
    const int left = static_cast<int>(b.left);
    const int top = static_cast<int>(b.top);
    const int right = static_cast<int>(b.right);
    const int bottom = static_cast<int>(b.bottom);
    return QRect(left, top, right - left, bottom - top);
}

bool CanvasWidget::cropToSelection() {
    if (!document_ || !selectionBox_.has_value()) {
        return false;
    }
    return document_->crop(selectionImageCropRect());
}

QPointF CanvasWidget::clampToImage(const QPointF &pos) const {
    const QSize imgSize = document_->current().size();
    const double dispW = imgSize.width() * displayScale_;
    const double dispH = imgSize.height() * displayScale_;
    return QPointF(clampAxis(pos.x(), displayOffset_.x(), displayOffset_.x() + dispW),
                    clampAxis(pos.y(), displayOffset_.y(), displayOffset_.y() + dispH));
}

CanvasWidget::SelectionHit CanvasWidget::hitTestSelection(const QPointF &pos) const {
    if (!selectionBox_.has_value()) {
        return SelectionHit::NoHit;
    }
    const QRectF box = selectionBox_->normalized();
    const double left = box.left();
    const double right = box.right();
    const double top = box.top();
    const double bottom = box.bottom();
    const double margin = kSelectionBorderMargin;

    if (std::abs(pos.x() - left) <= margin && pos.y() >= top - margin && pos.y() <= bottom + margin) {
        return SelectionHit::Left;
    }
    if (std::abs(pos.x() - right) <= margin && pos.y() >= top - margin && pos.y() <= bottom + margin) {
        return SelectionHit::Right;
    }
    if (std::abs(pos.y() - top) <= margin && pos.x() >= left - margin && pos.x() <= right + margin) {
        return SelectionHit::Top;
    }
    if (std::abs(pos.y() - bottom) <= margin && pos.x() >= left - margin && pos.x() <= right + margin) {
        return SelectionHit::Bottom;
    }
    if (pos.x() >= left && pos.x() <= right && pos.y() >= top && pos.y() <= bottom) {
        return SelectionHit::Inside;
    }
    return SelectionHit::NoHit;
}

void CanvasWidget::moveSelection(const QPointF &pos) {
    const QSize imgSize = document_->current().size();
    const double dispW = imgSize.width() * displayScale_;
    const double dispH = imgSize.height() * displayScale_;
    const QSizeF size = selectionSizeAtDragStart_;

    double newLeft = pos.x() - dragOffset_.x();
    double newTop = pos.y() - dragOffset_.y();
    newLeft = clampAxis(newLeft, displayOffset_.x(), displayOffset_.x() + dispW - size.width());
    newTop = clampAxis(newTop, displayOffset_.y(), displayOffset_.y() + dispH - size.height());

    selectionBox_ = QRectF(QPointF(newLeft, newTop), size);
    update();
    emit interactionChanged();
}

void CanvasWidget::resizeSelection(DragMode edge, const QPointF &pos) {
    const QSize imgSize = document_->current().size();
    const double dispW = imgSize.width() * displayScale_;
    const double dispH = imgSize.height() * displayScale_;
    const double minX = displayOffset_.x();
    const double minY = displayOffset_.y();
    const double maxX = minX + dispW;
    const double maxY = minY + dispH;

    QRectF box = *selectionBox_;
    double left = box.left();
    double top = box.top();
    double right = box.right();
    double bottom = box.bottom();

    switch (edge) {
        case DragMode::ResizeLeft:
            left = clampAxis(pos.x(), minX, right - kSelectionMinSize);
            break;
        case DragMode::ResizeRight:
            right = std::min(maxX, std::max(pos.x(), left + kSelectionMinSize));
            break;
        case DragMode::ResizeTop:
            top = clampAxis(pos.y(), minY, bottom - kSelectionMinSize);
            break;
        case DragMode::ResizeBottom:
            bottom = std::min(maxY, std::max(pos.y(), top + kSelectionMinSize));
            break;
        default:
            break;
    }

    selectionBox_ = QRectF(QPointF(left, top), QPointF(right, bottom));
    update();
    emit interactionChanged();
}

void CanvasWidget::growSelectionToMinSize() {
    const QSize imgSize = document_->current().size();
    const double dispW = imgSize.width() * displayScale_;
    const double dispH = imgSize.height() * displayScale_;
    const QRectF box = *selectionBox_;

    auto grow = [](double anchor, double moving, double lo, double hi) {
        if (std::abs(moving - anchor) >= kSelectionMinSize) {
            return moving;
        }
        const double direction = moving >= anchor ? 1.0 : -1.0;
        return clampAxis(anchor + direction * kSelectionMinSize, lo, hi);
    };

    const double x1 = grow(selectionStart_.x(), box.x() + box.width(), displayOffset_.x(), displayOffset_.x() + dispW);
    const double y1 = grow(selectionStart_.y(), box.y() + box.height(), displayOffset_.y(), displayOffset_.y() + dispH);
    selectionBox_ = QRectF(selectionStart_, QPointF(x1, y1));
}

// ---------- Straighten ----------

void CanvasWidget::toggleStraighten() {
    if (straightenActive_) {
        cancelStraighten();
    } else {
        startStraighten();
    }
}

void CanvasWidget::startStraighten() {
    if (!document_ || document_->current().isNull()) {
        return;
    }
    clearSelection();
    straightenActive_ = true;
    straightenAngle_ = 0.0;
    straightenDragHandle_ = StraightenHandle::NoHandle;
    document_->startStraighten();
    update();
    emit interactionChanged();
}

void CanvasWidget::cancelStraighten() {
    if (!straightenActive_) {
        return;
    }
    straightenActive_ = false;
    straightenAngle_ = 0.0;
    straightenDragHandle_ = StraightenHandle::NoHandle;
    document_->cancelStraighten();
    update();
    emit interactionChanged();
}

std::optional<QLineF> CanvasWidget::straightenLinePoints() const {
    if (!straightenActive_ || !document_ || document_->current().isNull()) {
        return std::nullopt;
    }
    const QSize imgSize = document_->current().size();
    const double dispW = imgSize.width() * displayScale_;
    const double dispH = imgSize.height() * displayScale_;
    const QPointF center(displayOffset_.x() + dispW / 2.0, displayOffset_.y() + dispH / 2.0);
    const double halfLen = dispW * 0.35;
    const double angleRad = qDegreesToRadians(straightenAngle_);
    const double dx = halfLen * std::cos(angleRad);
    const double dy = halfLen * std::sin(angleRad);
    return QLineF(center.x() - dx, center.y() - dy, center.x() + dx, center.y() + dy);
}

CanvasWidget::StraightenHandle CanvasWidget::straightenHitTest(const QPointF &pos) const {
    const std::optional<QLineF> line = straightenLinePoints();
    if (!line.has_value()) {
        return StraightenHandle::NoHandle;
    }
    const double margin = kStraightenHandleMargin;
    if (std::abs(pos.x() - line->x1()) <= margin && std::abs(pos.y() - line->y1()) <= margin) {
        return StraightenHandle::Left;
    }
    if (std::abs(pos.x() - line->x2()) <= margin && std::abs(pos.y() - line->y2()) <= margin) {
        return StraightenHandle::Right;
    }
    return StraightenHandle::NoHandle;
}

void CanvasWidget::updateStraightenAngle(const QPointF &pos) {
    const QSize imgSize = document_->current().size();
    const double dispW = imgSize.width() * displayScale_;
    const double dispH = imgSize.height() * displayScale_;
    const QPointF center(displayOffset_.x() + dispW / 2.0, displayOffset_.y() + dispH / 2.0);
    double dx = pos.x() - center.x();
    double dy = pos.y() - center.y();
    if (straightenDragHandle_ == StraightenHandle::Left) {
        dx = -dx;
        dy = -dy;
    }
    const double angle = qRadiansToDegrees(std::atan2(dy, dx));
    straightenAngle_ = clampAxis(angle, -kStraightenMaxAngle, kStraightenMaxAngle);
    update();
    emit interactionChanged();
}

// ---------- Mouse events ----------

void CanvasWidget::updateHoverCursor(const QPointF &pos) {
    if (straightenActive_) {
        setCursor(straightenHitTest(pos) != StraightenHandle::NoHandle ? Qt::SizeAllCursor : Qt::ArrowCursor);
        return;
    }
    switch (hitTestSelection(pos)) {
        case SelectionHit::Left:
        case SelectionHit::Right:
            setCursor(Qt::SizeHorCursor);
            break;
        case SelectionHit::Top:
        case SelectionHit::Bottom:
            setCursor(Qt::SizeVerCursor);
            break;
        default:
            setCursor(Qt::ArrowCursor);
            break;
    }
}

void CanvasWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton || !document_ || document_->current().isNull()) {
        QWidget::mousePressEvent(event);
        return;
    }
    const QPointF pos = event->position();

    if (straightenActive_) {
        const StraightenHandle hit = straightenHitTest(pos);
        if (hit == StraightenHandle::NoHandle) {
            cancelStraighten();
            return;
        }
        straightenDragHandle_ = hit;
        return;
    }

    const SelectionHit hit = hitTestSelection(pos);
    switch (hit) {
        case SelectionHit::Left:
        case SelectionHit::Right:
        case SelectionHit::Top:
        case SelectionHit::Bottom: {
            selectionBox_ = selectionBox_->normalized();
            dragMode_ = hit == SelectionHit::Left    ? DragMode::ResizeLeft
                        : hit == SelectionHit::Right  ? DragMode::ResizeRight
                        : hit == SelectionHit::Top    ? DragMode::ResizeTop
                                                       : DragMode::ResizeBottom;
            setCursor(hit == SelectionHit::Left || hit == SelectionHit::Right ? Qt::SizeHorCursor
                                                                               : Qt::SizeVerCursor);
            break;
        }
        case SelectionHit::Inside: {
            const QRectF norm = selectionBox_->normalized();
            dragMode_ = DragMode::Move;
            dragOffset_ = pos - norm.topLeft();
            selectionSizeAtDragStart_ = norm.size();
            setCursor(Qt::SizeAllCursor);
            break;
        }
        case SelectionHit::NoHit: {
            dragMode_ = DragMode::New;
            const QPointF clamped = clampToImage(pos);
            selectionStart_ = clamped;
            selectionBox_ = QRectF(clamped, clamped);
            update();
            break;
        }
    }
    emit interactionChanged();
}

void CanvasWidget::mouseMoveEvent(QMouseEvent *event) {
    if (!document_ || document_->current().isNull()) {
        QWidget::mouseMoveEvent(event);
        return;
    }
    const QPointF pos = event->position();
    const bool dragging =
        straightenActive_ ? (straightenDragHandle_ != StraightenHandle::NoHandle) : (dragMode_ != DragMode::Idle);
    if (!dragging) {
        updateHoverCursor(pos);
        return;
    }
    if (straightenActive_) {
        updateStraightenAngle(pos);
        return;
    }
    switch (dragMode_) {
        case DragMode::Move:
            moveSelection(pos);
            break;
        case DragMode::ResizeLeft:
        case DragMode::ResizeRight:
        case DragMode::ResizeTop:
        case DragMode::ResizeBottom:
            resizeSelection(dragMode_, pos);
            break;
        case DragMode::New: {
            const QPointF clamped = clampToImage(pos);
            selectionBox_ = QRectF(selectionStart_, clamped);
            update();
            emit interactionChanged();
            break;
        }
        default:
            break;
    }
}

void CanvasWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton) {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    if (straightenActive_) {
        if (straightenDragHandle_ != StraightenHandle::NoHandle) {
            document_->applyStraighten(straightenAngle_, /*keepBase=*/true);
            straightenAngle_ = 0.0;
        }
        straightenDragHandle_ = StraightenHandle::NoHandle;
        update();
        updateHoverCursor(event->position());
        emit interactionChanged();
        return;
    }
    if (dragMode_ == DragMode::New && selectionBox_.has_value()) {
        const QRectF box = *selectionBox_;
        if (std::abs(box.width()) < kSelectionClickThreshold || std::abs(box.height()) < kSelectionClickThreshold) {
            clearSelection();
        } else {
            growSelectionToMinSize();
        }
        update();
    }
    dragMode_ = DragMode::Idle;
    updateHoverCursor(event->position());
    emit interactionChanged();
}

void CanvasWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    if (straightenActive_ || !document_ || document_->current().isNull()) {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }
    if (hitTestSelection(event->position()) == SelectionHit::Inside) {
        emit processAndSaveRequested();
    }
}
