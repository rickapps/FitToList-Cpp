// Draws the MSIX package logos (Square44x44Logo, Square150x150Logo,
// StoreLogo) procedurally with QPainter, the same no-bundled-image-assets
// approach ToolbarIcons.cpp uses for the in-app toolbar.
#include <QDir>
#include <QGuiApplication>
#include <QImage>
#include <QList>
#include <QPainter>
#include <QPair>
#include <QString>

namespace {

QImage renderIcon(int size) {
    QImage image(size, size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);

    const qreal margin = size * 0.08;
    const QRectF bounds(margin, margin, size - 2 * margin, size - 2 * margin);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0x2f, 0x6f, 0xa8));
    painter.drawRoundedRect(bounds, size * 0.18, size * 0.18);

    // Crop-corner brackets, echoing the app's crop tool.
    const qreal armLength = size * 0.28;
    const qreal thickness = qMax<qreal>(1.5, size * 0.07);
    const qreal inset = size * 0.28;
    painter.setPen(QPen(Qt::white, thickness, Qt::SolidLine, Qt::SquareCap));

    painter.drawLine(QPointF(inset, inset), QPointF(inset + armLength, inset));
    painter.drawLine(QPointF(inset, inset), QPointF(inset, inset + armLength));

    painter.drawLine(QPointF(size - inset, size - inset), QPointF(size - inset - armLength, size - inset));
    painter.drawLine(QPointF(size - inset, size - inset), QPointF(size - inset, size - inset - armLength));

    return image;
}

}

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    if (argc != 2) {
        return 1;
    }

    const QString outDir = QString::fromLocal8Bit(argv[1]);
    QDir().mkpath(outDir);

    const QList<QPair<QString, int>> assets = {
        {"Square44x44Logo.png", 44},
        {"Square150x150Logo.png", 150},
        {"StoreLogo.png", 50},
    };

    for (const auto &asset : assets) {
        if (!renderIcon(asset.second).save(outDir + "/" + asset.first)) {
            return 1;
        }
    }

    return 0;
}
