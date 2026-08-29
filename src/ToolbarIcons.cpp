#include "ToolbarIcons.h"

#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QtMath>
#include <cmath>
#include <functional>

namespace {

QPointF pointOnCircle(const QPointF &center, double r, double angleDeg) {
    const double rad = qDegreesToRadians(angleDeg);
    return {center.x() + r * std::cos(rad), center.y() + r * std::sin(rad)};
}

void fillTriangle(QPainter &painter, const QPointF &a, const QPointF &b, const QPointF &c, const QColor &fill) {
    QPainterPath path;
    path.moveTo(a);
    path.lineTo(b);
    path.lineTo(c);
    path.closeSubpath();
    painter.fillPath(path, fill);
}

// PIL's arc() rasterizes a smooth curve; QPainter::drawArc's angle
// convention (counterclockwise, 16ths of a degree) is easy to get backwards
// against Python's plain clockwise degrees, so this just samples points with
// the identical cos/sin formula pointOnCircle uses and draws the polyline -
// guaranteed to match, and visually indistinguishable from a true arc at
// icon scale.
void drawArcPolyline(QPainter &painter, const QPointF &center, double r, double startDeg, double endDeg,
                      const QPen &pen) {
    constexpr int kSegments = 32;
    painter.setPen(pen);
    QPointF previous = pointOnCircle(center, r, startDeg);
    for (int i = 1; i <= kSegments; ++i) {
        const double angle = startDeg + (endDeg - startDeg) * i / kSegments;
        const QPointF current = pointOnCircle(center, r, angle);
        painter.drawLine(previous, current);
        previous = current;
    }
}

void drawArrowhead(QPainter &painter, const QPointF &tip, double dirAngleDeg, double headSize, const QColor &fill) {
    const QPointF backLeft = pointOnCircle(tip, headSize, dirAngleDeg + 150);
    const QPointF backRight = pointOnCircle(tip, headSize, dirAngleDeg - 150);
    fillTriangle(painter, tip, backLeft, backRight, fill);
}

void drawArrow(QPainter &painter, const QPointF &tail, const QPointF &tip, double headSize, const QPen &linePen,
               const QColor &headFill) {
    painter.setPen(linePen);
    painter.drawLine(tail, tip);
    const double dirAngle = qRadiansToDegrees(std::atan2(tip.y() - tail.y(), tip.x() - tail.x()));
    drawArrowhead(painter, tip, dirAngle, headSize, headFill);
}

void paintFolder(QPainter &painter, int size, const QColor &fg) {
    const double pad = size * 0.11;
    const double tabW = size * 0.4;
    const double tabH = size * 0.11;
    const double bodyTop = pad + tabH + size * 0.04;
    painter.setPen(QPen(fg, 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(QRectF(pad, pad, tabW, bodyTop - pad));
    painter.drawRect(QRectF(pad, bodyTop, size - 2 * pad, size - pad - bodyTop));
}

void paintOpenFolder(QPainter &painter, int size, const QColor &fg) {
    paintFolder(painter, size, fg);
    const QPointF tail(size * 0.42, size * 0.68);
    const QPointF tip(size * 0.86, size * 0.24);
    drawArrow(painter, tail, tip, size * 0.2, QPen(fg, 3), fg);
}

void paintRotate(QPainter &painter, int size, const QColor &fg, bool clockwise) {
    const QPointF center(size / 2.0, size / 2.0);
    const double r = size * 0.32;
    double start, end, tipAngle, dirAngle;
    if (clockwise) {
        start = 40;
        end = 300;
        tipAngle = end;
        dirAngle = end + 90;
    } else {
        start = 240;
        end = 500;
        tipAngle = start;
        dirAngle = start + 90;
    }
    drawArcPolyline(painter, center, r, start, end, QPen(fg, 3));
    const QPointF tip = pointOnCircle(center, r, tipAngle);
    drawArrowhead(painter, tip, dirAngle, size * 0.21, fg);
}

void paintCrop(QPainter &painter, int size, const QColor &fg) {
    const double pad = size * 0.14;
    const double arm = size * 0.35;
    painter.setPen(QPen(fg, 3));
    painter.drawPolyline(QPolygonF({QPointF(pad, pad + arm), QPointF(pad, pad), QPointF(pad + arm, pad)}));
    painter.drawPolyline(QPolygonF({QPointF(size - pad, size - pad - arm), QPointF(size - pad, size - pad),
                                     QPointF(size - pad - arm, size - pad)}));
}

void paintStraighten(QPainter &painter, int size, const QColor &fg) {
    const double pad = size * 0.22;
    const double tilt = size * 0.1;
    const QPointF p0(pad, size - pad - tilt);
    const QPointF p1(size - pad, pad + tilt);
    painter.setPen(QPen(fg, 3));
    painter.drawLine(p0, p1);

    const double r = size * 0.045;
    painter.setPen(Qt::NoPen);
    painter.setBrush(fg);
    painter.drawEllipse(p0, r, r);
    painter.drawEllipse(p1, r, r);

    const double arrowLen = size * 0.24;
    const double head = size * 0.06;
    const struct {
        QPointF handle;
        int direction;
    } handles[] = {{p0, -1}, {p1, 1}};
    for (const auto &h : handles) {
        const QPointF tip(h.handle.x(), h.handle.y() + h.direction * arrowLen);
        painter.setPen(QPen(fg, 2));
        painter.drawLine(h.handle, tip);
        const double baseAngle = h.direction < 0 ? 90 : 270;
        drawArrowhead(painter, tip, baseAngle, head, fg);
    }
}

void paintSave(QPainter &painter, int size, const QColor &fg) {
    const double pad = size * 0.11;
    painter.setPen(QPen(fg, 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(QRectF(pad, pad, size - 2 * pad, size - 2 * pad));
    painter.setPen(Qt::NoPen);
    painter.setBrush(fg);
    painter.drawRect(QRectF(pad + 4, pad, size - 2 * pad - 8, 6));
    painter.setPen(QPen(fg, 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(QRectF(pad + 5, size - pad - 9, size - 2 * pad - 10, 8));
}

QIcon makeIcon(int size, const std::function<void(QPainter &)> &paint) {
    QImage image(size, size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    paint(painter);
    painter.end();
    return QIcon(QPixmap::fromImage(image));
}

}  // namespace

QIcon folderIcon(int size, const QColor &fg) {
    return makeIcon(size, [&](QPainter &p) { paintFolder(p, size, fg); });
}

QIcon openFolderIcon(int size, const QColor &fg) {
    return makeIcon(size, [&](QPainter &p) { paintOpenFolder(p, size, fg); });
}

QIcon rotateRightIcon(int size, const QColor &fg) {
    return makeIcon(size, [&](QPainter &p) { paintRotate(p, size, fg, /*clockwise=*/true); });
}

QIcon rotateLeftIcon(int size, const QColor &fg) {
    return makeIcon(size, [&](QPainter &p) { paintRotate(p, size, fg, /*clockwise=*/false); });
}

QIcon cropIcon(int size, const QColor &fg) {
    return makeIcon(size, [&](QPainter &p) { paintCrop(p, size, fg); });
}

QIcon straightenIcon(int size, const QColor &fg) {
    return makeIcon(size, [&](QPainter &p) { paintStraighten(p, size, fg); });
}

QIcon saveIcon(int size, const QColor &fg) {
    return makeIcon(size, [&](QPainter &p) { paintSave(p, size, fg); });
}
