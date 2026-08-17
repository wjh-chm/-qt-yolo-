#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QString>

#include <array>

struct StorageSettings
{
    QString recordRoot;
    int intervalSeconds;
    std::array<QString, 4> channelNames;
    std::array<int, 4> channelIndexes;
};

struct DatabaseSettings
{
    QString driver;
    QString host;
    int port;
    QString databaseName;
    QString userName;
    QString password;
};

class AppSettings
{
public:
    static StorageSettings loadStorageSettings();
    static bool saveStorageSettings(const StorageSettings &settings);
    static DatabaseSettings loadDatabaseSettings();
    static bool ensureDefaultDatabaseSettings();

    static QString settingsFilePath();
    static QString databaseSettingsFilePath();
    static QString defaultRecordRoot();
    static int defaultIntervalSeconds();
    static QString defaultChannelName(int index);
    static DatabaseSettings defaultDatabaseSettings();
};

#endif // APPSETTINGS_H
