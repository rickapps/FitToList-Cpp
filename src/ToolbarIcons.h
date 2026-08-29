#pragma once

#include <QColor>
#include <QIcon>

// Procedurally-drawn toolbar icons, replacing photo_editor.py's PIL
// ImageDraw-based ones (_draw_*_icon / build_toolbar_icons) with QPainter
// equivalents. No bundled image assets either way.

constexpr int kToolbarIconSize = 28;    // px, source size for generated icons
constexpr int kToolbarButtonSize = 36;  // px, fixed width/height for every toolbar button
inline const QColor kToolbarIconColor = QColor("#333333");

QIcon folderIcon(int size = kToolbarIconSize, const QColor &fg = kToolbarIconColor);
QIcon openFolderIcon(int size = kToolbarIconSize, const QColor &fg = kToolbarIconColor);
QIcon rotateRightIcon(int size = kToolbarIconSize, const QColor &fg = kToolbarIconColor);
QIcon rotateLeftIcon(int size = kToolbarIconSize, const QColor &fg = kToolbarIconColor);
QIcon cropIcon(int size = kToolbarIconSize, const QColor &fg = kToolbarIconColor);
QIcon straightenIcon(int size = kToolbarIconSize, const QColor &fg = kToolbarIconColor);
QIcon saveIcon(int size = kToolbarIconSize, const QColor &fg = kToolbarIconColor);
