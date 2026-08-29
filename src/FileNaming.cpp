#include "FileNaming.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <algorithm>

const QSet<QString> kImageExtensions = {
    ".png", ".jpg", ".jpeg", ".gif", ".bmp", ".tiff", ".webp",
};

namespace {

// Splits a filename the way Python's os.path.splitext does: root is
// everything before the last dot, ext is the last dot plus what follows.
void splitNameExt(const QString &fileName, QString *root, QString *ext) {
    const QFileInfo info(fileName);
    *root = info.completeBaseName();
    const QString suffix = info.suffix();
    *ext = suffix.isEmpty() ? QString() : "." + suffix;
}

QRegularExpression suffixPattern(const QString &root, const QString &ext) {
    const QString pattern =
        "^" + QRegularExpression::escape(root) + "_(\\d+)" + QRegularExpression::escape(ext) + "$";
    return QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
}

}  // namespace

bool isImageFile(const QString &fileName) {
    return kImageExtensions.contains(QFileInfo(fileName).suffix().prepend('.').toLower());
}

int nextSuffix(const QString &targetFolder, const QString &root, const QString &ext) {
    const QRegularExpression pattern = suffixPattern(root, ext);
    int maxSuffix = -1;
    const QStringList names = QDir(targetFolder).entryList(QDir::Files);
    for (const QString &name : names) {
        const QRegularExpressionMatch match = pattern.match(name);
        if (match.hasMatch()) {
            maxSuffix = std::max(maxSuffix, match.captured(1).toInt());
        }
    }
    return maxSuffix + 1;
}

QStringList matchingProcessedNames(const QString &sourceName, const QStringList &processedNames) {
    QString root, ext;
    splitNameExt(sourceName, &root, &ext);
    const QRegularExpression pattern = suffixPattern(root, ext);

    QList<QPair<int, QString>> matches;
    for (const QString &name : processedNames) {
        const QRegularExpressionMatch match = pattern.match(name);
        if (match.hasMatch()) {
            matches.append({match.captured(1).toInt(), name});
        }
    }
    std::sort(matches.begin(), matches.end(), [](const auto &a, const auto &b) {
        return a.first < b.first;
    });

    QStringList result;
    result.reserve(matches.size());
    for (const auto &[suffix, name] : matches) {
        result.append(name);
    }
    return result;
}
