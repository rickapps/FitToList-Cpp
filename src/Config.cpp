#include "Config.h"

#include <QSettings>
#include <QVariant>

AppConfig loadConfig(QSettings &settings) {
    AppConfig config;
    config.sourceFolder = settings.value("sourceFolder").toString();
    config.targetFolder = settings.value("targetFolder").toString();
    config.maxSizeEnabled = settings.value("maxSizeEnabled", false).toBool();
    config.maxWidth = settings.value("maxWidth", 0).toInt();
    config.maxHeight = settings.value("maxHeight", 0).toInt();
    return config;
}

void saveConfig(const AppConfig &config, QSettings &settings) {
    settings.setValue("sourceFolder", config.sourceFolder);
    settings.setValue("targetFolder", config.targetFolder);
    settings.setValue("maxSizeEnabled", config.maxSizeEnabled);
    settings.setValue("maxWidth", config.maxWidth);
    settings.setValue("maxHeight", config.maxHeight);
}
