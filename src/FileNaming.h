#pragma once

#include <QSet>
#include <QString>
#include <QStringList>

// Recognized image file extensions, lowercase with leading dot.
extern const QSet<QString> kImageExtensions;

// True if fileName's extension is one of kImageExtensions (case-insensitive).
bool isImageFile(const QString &fileName);

// Next "_NN" suffix for root/ext among files already in targetFolder, i.e. one
// past the highest existing "{root}_NN{ext}" match, or 0 if there are none.
int nextSuffix(const QString &targetFolder, const QString &root, const QString &ext);

// Names in processedNames matching sourceName's "{root}_NN{ext}" convention
// (the same pattern nextSuffix scans for), sorted by numeric suffix ascending.
QStringList matchingProcessedNames(const QString &sourceName, const QStringList &processedNames);
