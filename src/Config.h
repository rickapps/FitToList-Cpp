#pragma once

#include <QString>

class QSettings;

// Persisted app settings - ports load_config/save_config's fields (originally
// hand-rolled JSON at ~/.fittolist/config.json) onto QSettings, which handles
// the per-platform storage location and format itself.
struct AppConfig {
    QString sourceFolder;
    QString targetFolder;
    bool maxSizeEnabled = false;
    int maxWidth = 0;
    int maxHeight = 0;
};

AppConfig loadConfig(QSettings &settings);
void saveConfig(const AppConfig &config, QSettings &settings);
