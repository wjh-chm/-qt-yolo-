#ifndef CLIENTAPI_H
#define CLIENTAPI_H

#include <QObject>
#include <QJsonObject>
#include <QTcpSocket>
#include <QJsonArray>
#include <QList>

class ClientApi : public QObject
{
    Q_OBJECT

public:
    explicit ClientApi(QObject *parent = nullptr);

    void connectToServer(const QString &host, quint16 port);
    bool isConnected() const;
    qint64  login(const QString &username, const QString &password);
    qint64 queryVideoList(const QString &scope,
                        const QString &date,
                        int channelId,
                        int page,
                        int pageSize);

    qint64  queryImageList(const QString &scope,
                        const QString &date,
                        int channelId,
                        int page,
                        int pageSize);

    qint64  queryLogList(const QString &mode,
                      int page,
                      int pageSize);
    qint64  findRelatedVideo(const QString &scope,
                          int channelId,
                          const QString &captureTime);
    qint64  insertVideoRecord(int channelId,
                           const QString &videoName,
                           const QString &videoPath,
                           const QString &startTime,
                           const QString &endTime,
                           const QString &createTime,
                           int exceptionId = -1);
    qint64  insertAnomalySession(const QJsonObject &exceptionData,
                              const QJsonObject &videoData,
                              const QJsonArray &images);
    qint64  insertImageRecord(int channelId,
                           const QString &imageName,
                           const QString &imagePath,
                           const QString &captureTime,
                           const QString &createTime,
                           int exceptionId = -1);
    qint64  deleteImagesByIds(const QList<int> &imageIds);

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &message);
    void loginFinished(qint64 requestId,bool success, const QString &message);
    void videoListFinished(qint64 requestId,bool success, const QJsonArray &list, int totalCount, const QString &message);
    void imageListFinished(qint64 requestId,bool success, const QJsonArray &list, int totalCount, const QString &message);
    void relatedVideoFinished(qint64 requestId,bool success, const QJsonObject &record, const QString &message);
    void insertVideoFinished(qint64 requestId,bool success, int videoId, const QString &message);
    void anomalySessionFinished(qint64 requestId,bool success, int exceptionId, const QString &message);
    void insertImageFinished(qint64 requestId,bool success, int imageId, const QString &message);
    void deleteImagesFinished(qint64 requestId,bool success, const QString &message);
    void logListFinished(qint64 requestId,bool success,
                         const QJsonArray &list,
                         int totalCount,
                         const QString &message);
private slots:
    void onReadyRead();

private:
    void sendJson(const QJsonObject &object);
    void handleResponse(const QJsonObject &object);
    qint64 nextRequestId();
    QJsonObject makeRequest(const QString &type, const QJsonObject &data);
private:
    QTcpSocket *m_socket = nullptr;
    QByteArray m_buffer;
    qint64 m_nextRequestId = 1;
    QString m_token;
};

#endif
