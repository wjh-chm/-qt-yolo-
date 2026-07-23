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

QString AppSettings::settingsFilePath()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath("config.ini");
}

QString AppSettings::defaultRecordRoot()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath("records");
}

int AppSettings::defaultIntervalSeconds()
{
    return 30;
}

QString AppSettings::defaultChannelName(int index)
{
    return QStringLiteral("测试通道名%1").arg(index + 1);
}
