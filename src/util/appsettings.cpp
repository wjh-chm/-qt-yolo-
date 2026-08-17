#include "appsettings.h"

#include <QCoreApplication>
#include <QDir>
#include <QSettings>

namespace
{
StorageSettings defaultStorageSettings()
{
    StorageSettings settings;
    settings.recordRoot = AppSettings::defaultRecordRoot();
    settings.intervalSeconds = AppSettings::defaultIntervalSeconds();
    for (int i = 0; i < 4; ++i) {
        settings.channelNames[i] = AppSettings::defaultChannelName(i);
        settings.channelIndexes[i] = i;
    }
    return settings;
}

bool isMissingDatabaseConfig(const QSettings &ini)
{
    return !ini.contains("database/driver")
           || !ini.contains("database/host")
           || !ini.contains("database/port")
           || !ini.contains("database/name")
           || !ini.contains("database/user")
           || !ini.contains("database/password");
}
}

StorageSettings AppSettings::loadStorageSettings()
{
    StorageSettings settings = defaultStorageSettings();
    QSettings ini(settingsFilePath(), QSettings::IniFormat);

    settings.recordRoot = ini.value("storage/recordRoot", settings.recordRoot).toString().trimmed();
    settings.intervalSeconds = ini.value("storage/intervalSeconds", settings.intervalSeconds).toInt();

    if (settings.recordRoot.isEmpty()) {
        settings.recordRoot = defaultRecordRoot();
    }
    if (QDir::isRelativePath(settings.recordRoot)) {
        settings.recordRoot = QDir(QCoreApplication::applicationDirPath()).filePath(settings.recordRoot);
    }
    if (settings.intervalSeconds <= 0) {
        settings.intervalSeconds = defaultIntervalSeconds();
    }

    for (int i = 0; i < 4; ++i) {
        const QString nameKey = QString("channels/name%1").arg(i + 1);
        const QString indexKey = QString("channels/index%1").arg(i + 1);

        settings.channelNames[i] = ini.value(nameKey, settings.channelNames[i]).toString().trimmed();
        settings.channelIndexes[i] = ini.value(indexKey, settings.channelIndexes[i]).toInt();

        if (settings.channelNames[i].isEmpty()) {
            settings.channelNames[i] = defaultChannelName(i);
        }
        if (settings.channelIndexes[i] < 0 || settings.channelIndexes[i] > 3) {
            settings.channelIndexes[i] = i;
        }
    }

    return settings;
}

bool AppSettings::saveStorageSettings(const StorageSettings &settings)
{
    QSettings ini(settingsFilePath(), QSettings::IniFormat);
    ini.setValue("storage/recordRoot", settings.recordRoot);
    ini.setValue("storage/intervalSeconds", settings.intervalSeconds);

    for (int i = 0; i < 4; ++i) {
        ini.setValue(QString("channels/name%1").arg(i + 1), settings.channelNames[i]);
        ini.setValue(QString("channels/index%1").arg(i + 1), settings.channelIndexes[i]);
    }

    ini.sync();
    return ini.status() == QSettings::NoError;
}

DatabaseSettings AppSettings::loadDatabaseSettings()
{
    ensureDefaultDatabaseSettings();

    const DatabaseSettings defaults = defaultDatabaseSettings();
    QSettings ini(databaseSettingsFilePath(), QSettings::IniFormat);

    DatabaseSettings settings;
    settings.driver = ini.value("database/driver", defaults.driver).toString().trimmed();
    settings.host = ini.value("database/host", defaults.host).toString().trimmed();
    settings.port = ini.value("database/port", defaults.port).toInt();
    settings.databaseName = ini.value("database/name", defaults.databaseName).toString().trimmed();
    settings.userName = ini.value("database/user", defaults.userName).toString().trimmed();
    settings.password = ini.value("database/password", defaults.password).toString();

    if (settings.driver.isEmpty()) {
        settings.driver = defaults.driver;
    }
    if (settings.host.isEmpty()) {
        settings.host = defaults.host;
    }
    if (settings.port <= 0) {
        settings.port = defaults.port;
    }
    if (settings.databaseName.isEmpty()) {
        settings.databaseName = defaults.databaseName;
    }
    if (settings.userName.isEmpty()) {
        settings.userName = defaults.userName;
    }

    return settings;
}

bool AppSettings::ensureDefaultDatabaseSettings()
{
    QSettings ini(databaseSettingsFilePath(), QSettings::IniFormat);
    if (!isMissingDatabaseConfig(ini)) {
        return ini.status() == QSettings::NoError;
    }

    const DatabaseSettings defaults = defaultDatabaseSettings();
    if (!ini.contains("database/driver")) {
        ini.setValue("database/driver", defaults.driver);
    }
    if (!ini.contains("database/host")) {
        ini.setValue("database/host", defaults.host);
    }
    if (!ini.contains("database/port")) {
        ini.setValue("database/port", defaults.port);
    }
    if (!ini.contains("database/name")) {
        ini.setValue("database/name", defaults.databaseName);
    }
    if (!ini.contains("database/user")) {
        ini.setValue("database/user", defaults.userName);
    }
    if (!ini.contains("database/password")) {
        ini.setValue("database/password", defaults.password);
    }

    ini.sync();
    return ini.status() == QSettings::NoError;
}

QString AppSettings::settingsFilePath()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath("config.ini");
}

QString AppSettings::databaseSettingsFilePath()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath("db.ini");
}

QString AppSettings::defaultRecordRoot()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath("records");
}

int AppSettings::defaultIntervalSeconds()
{
    return 30;
}

DatabaseSettings AppSettings::defaultDatabaseSettings()
{
    DatabaseSettings settings;
    settings.driver = QStringLiteral("QMYSQL");
    settings.host = QStringLiteral("127.0.0.1");
    settings.port = 3306;
    settings.databaseName = QStringLiteral("smart_minitor");
    settings.userName = QStringLiteral("lili");
    settings.password = QStringLiteral("123456");
    return settings;
}

QString AppSettings::defaultChannelName(int index)
{
    return QStringLiteral("测试通道名%1").arg(index + 1);
}
