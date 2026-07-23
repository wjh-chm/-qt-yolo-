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

class AppSettings
{
public:
    static StorageSettings loadStorageSettings();
    static bool saveStorageSettings(const StorageSettings &settings);

    static QString settingsFilePath();
    static QString defaultRecordRoot();
    static int defaultIntervalSeconds();
    static QString defaultChannelName(int index);
};

#endif // APPSETTINGS_H
