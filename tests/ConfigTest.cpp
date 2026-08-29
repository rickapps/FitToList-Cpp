#include <QSettings>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Config.h"

class ConfigTest : public QObject {
    Q_OBJECT

private slots:
    void roundTripsAllFields() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QSettings settings(dir.path() + "/config.ini", QSettings::IniFormat);

        AppConfig config;
        config.sourceFolder = "/photos/source";
        config.targetFolder = "/photos/processed";
        config.maxSizeEnabled = true;
        config.maxWidth = 1600;
        config.maxHeight = 1200;
        saveConfig(config, settings);

        QSettings reloaded(dir.path() + "/config.ini", QSettings::IniFormat);
        const AppConfig result = loadConfig(reloaded);
        QCOMPARE(result.sourceFolder, config.sourceFolder);
        QCOMPARE(result.targetFolder, config.targetFolder);
        QCOMPARE(result.maxSizeEnabled, config.maxSizeEnabled);
        QCOMPARE(result.maxWidth, config.maxWidth);
        QCOMPARE(result.maxHeight, config.maxHeight);
    }

    void missingSettingsProduceSensibleDefaults() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QSettings settings(dir.path() + "/empty.ini", QSettings::IniFormat);

        const AppConfig result = loadConfig(settings);
        QVERIFY(result.sourceFolder.isEmpty());
        QVERIFY(result.targetFolder.isEmpty());
        QVERIFY(!result.maxSizeEnabled);
        QCOMPARE(result.maxWidth, 0);
        QCOMPARE(result.maxHeight, 0);
    }
};

QTEST_MAIN(ConfigTest)
#include "ConfigTest.moc"
