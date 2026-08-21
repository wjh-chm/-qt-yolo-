#include "clientapi.h"
#include <QNetworkProxy>
#include <QDataStream>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonValue>

ClientApi::ClientApi(QObject *parent)
    : QObject(parent)
{
    m_socket = new QTcpSocket(this);
    m_socket->setProxy(QNetworkProxy::NoProxy);
    connect(m_socket, &QTcpSocket::connected,
            this, &ClientApi::connected);

    connect(m_socket, &QTcpSocket::disconnected,
            this, &ClientApi::disconnected);

    connect(m_socket, &QTcpSocket::readyRead,
            this, &ClientApi::onReadyRead);

    connect(m_socket, &QTcpSocket::errorOccurred,
            this, [this](QAbstractSocket::SocketError) {
                emit errorOccurred(m_socket->errorString());
            });
}

void ClientApi::connectToServer(const QString &host, quint16 port)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        return;
    }

    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->abort();
    }

    m_socket->connectToHost(host, port);
}

bool ClientApi::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

qint64 ClientApi::login(const QString &username, const QString &password)
{
    QJsonObject data;
    data["username"] = username;
    data["password"] = password;

    QJsonObject request = makeRequest("LOGIN_REQ", data);
    sendJson(request);

    return request.value("request_id").toInteger();
}
qint64 ClientApi::queryVideoList(const QString &scope,
                                 const QString &date,
                                 int channelId,
                                 int page,
                                 int pageSize)
{

    QJsonObject data;
    data["scope"] = scope;
    data["date"] = date;
    data["channel_id"] = channelId;
    data["page"] = page;
    data["page_size"] = pageSize;

    QJsonObject request = makeRequest("QUERY_VIDEO_LIST_REQ", data);
    sendJson(request);

    return request.value("request_id").toInteger();
}

qint64 ClientApi::queryImageList(const QString &scope,
                               const QString &date,
                               int channelId,
                               int page,
                               int pageSize)
{
    QJsonObject data;
    data["scope"] = scope;
    data["date"] = date;
    data["channel_id"] = channelId;
    data["page"] = page;
    data["page_size"] = pageSize;

    QJsonObject request = makeRequest("QUERY_IMAGE_LIST_REQ", data);
    sendJson(request);

    return request.value("request_id").toInteger();
}

qint64 ClientApi::queryLogList(const QString &mode,
                             int page,
                             int pageSize)
{
    QJsonObject data;
    data["mode"] = mode;
    data["page"] = page;
    data["page_size"] = pageSize;


    QJsonObject request = makeRequest("QUERY_LOG_LIST_REQ", data);
    sendJson(request);

    return request.value("request_id").toInteger();
}

qint64 ClientApi::findRelatedVideo(const QString &scope,
                                 int channelId,
                                 const QString &captureTime)
{
    QJsonObject data;
    data["scope"] = scope;
    data["channel_id"] = channelId;
    data["capture_time"] = captureTime;

    QJsonObject request = makeRequest("FIND_RELATED_VIDEO_REQ", data);
    sendJson(request);

    return request.value("request_id").toInteger();
}

qint64 ClientApi::insertVideoRecord(int channelId,
                                  const QString &videoName,
                                  const QString &videoPath,
                                  const QString &startTime,
                                  const QString &endTime,
                                  const QString &createTime,
                                  int exceptionId)
{
    QJsonObject data;
    data["channel_id"] = channelId;
    data["video_name"] = videoName;
    data["video_path"] = videoPath;
    data["start_time"] = startTime;
    data["end_time"] = endTime;
    data["create_time"] = createTime;

    if (exceptionId > 0) {
        data["exception_id"] = exceptionId;
    } else {
        data["exception_id"] = QJsonValue::Null;
    }

    QJsonObject request = makeRequest("INSERT_VIDEO_REQ", data);
    sendJson(request);

    return request.value("request_id").toInteger();
}

qint64 ClientApi::insertAnomalySession(const QJsonObject &exceptionData,
                                     const QJsonObject &videoData,
                                     const QJsonArray &images)
{
    QJsonObject data;
    data["exception"] = exceptionData;
    data["video"] = videoData;
    data["images"] = images;

    QJsonObject request = makeRequest("INSERT_ANOMALY_SESSION_REQ", data);
    sendJson(request);

    return request.value("request_id").toInteger();
}

qint64 ClientApi::insertImageRecord(int channelId,
                                  const QString &imageName,
                                  const QString &imagePath,
                                  const QString &captureTime,
                                  const QString &createTime,
                                  int exceptionId)
{
    QJsonObject data;
    data["channel_id"] = channelId;
    data["image_name"] = imageName;
    data["image_path"] = imagePath;
    data["capture_time"] = captureTime;
    data["create_time"] = createTime;

    if (exceptionId > 0) {
        data["exception_id"] = exceptionId;
    } else {
        data["exception_id"] = QJsonValue::Null;
    }

    QJsonObject request = makeRequest("INSERT_IMAGE_REQ", data);
    sendJson(request);

    return request.value("request_id").toInteger();
}

qint64 ClientApi::deleteImagesByIds(const QList<int> &imageIds)
{
    QJsonArray idArray;
    for (int imageId : imageIds) {
        if (imageId > 0) {
            idArray.append(imageId);
        }
    }

    QJsonObject data;
    data["image_ids"] = idArray;

    QJsonObject request = makeRequest("DELETE_IMAGES_REQ", data);
    sendJson(request);

    return request.value("request_id").toInteger();
}

void ClientApi::sendJson(const QJsonObject &object)
{
    if (!isConnected()) {
        emit errorOccurred(QStringLiteral("服务端未连接"));
        return;
    }

    QByteArray body = QJsonDocument(object).toJson(QJsonDocument::Compact);

    QByteArray packet;

    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << static_cast<quint32>(body.size());

    packet.append(body);

    m_socket->write(packet);
}

void ClientApi::onReadyRead()
{
    m_buffer.append(m_socket->readAll());

    while (true) {
        if (m_buffer.size() < 4) {
            return;
        }

        QDataStream stream(m_buffer.left(4));
        stream.setByteOrder(QDataStream::BigEndian);

        quint32 bodyLen = 0;
        stream >> bodyLen;

        if (bodyLen == 0 || bodyLen > 1024 * 1024) {
            m_buffer.clear();
            emit errorOccurred(QStringLiteral("服务端响应长度异常"));
            return;
        }

        if (m_buffer.size() < 4 + static_cast<int>(bodyLen)) {
            return;
        }

        QByteArray body = m_buffer.mid(4, bodyLen);
        m_buffer.remove(0, 4 + bodyLen);

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(body, &err);

        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            emit errorOccurred(QStringLiteral("服务端响应 JSON 解析失败"));
            continue;
        }

        handleResponse(doc.object());
    }
}

void ClientApi::handleResponse(const QJsonObject &object)
{
    QString type = object.value("type").toString();
    int code = object.value("code").toInt();
    QString message = object.value("message").toString();
    qint64 requestId = object.value("request_id").toInteger();

    if (type == "LOGIN_RESP") {
        emit loginFinished(requestId, code == 0, message);
        return;
    }

    if (type == "QUERY_VIDEO_LIST_RESP") {
        QJsonObject data = object.value("data").toObject();
        QJsonArray list = data.value("list").toArray();
        int totalCount = data.value("total_count").toInt(list.size());
        emit videoListFinished(requestId, code == 0, list, totalCount, message);
        return;
    }

    if (type == "QUERY_IMAGE_LIST_RESP") {
        QJsonObject data = object.value("data").toObject();
        QJsonArray list = data.value("list").toArray();
        int totalCount = data.value("total_count").toInt(list.size());
        emit imageListFinished(requestId, code == 0, list, totalCount, message);
        return;
    }

    if (type == "FIND_RELATED_VIDEO_RESP") {
        const QJsonObject data = object.value("data").toObject();
        QJsonObject record = data.value("record").toObject();
        if (record.isEmpty() && data.contains("video_path")) {
            record = data;
        }
        emit relatedVideoFinished(requestId, code == 0, record, message);
        return;
    }

    if (type == "INSERT_VIDEO_RESP") {
        QJsonObject data = object.value("data").toObject();
        const int videoId = data.value("id").toInt();
        emit insertVideoFinished(requestId, code == 0, videoId, message);
        return;
    }

    if (type == "INSERT_ANOMALY_SESSION_RESP") {
        QJsonObject data = object.value("data").toObject();
        const int exceptionId = data.value("exception_id").toInt();
        emit anomalySessionFinished(requestId, code == 0, exceptionId, message);
        return;
    }

    if (type == "INSERT_IMAGE_RESP") {
        QJsonObject data = object.value("data").toObject();
        const int imageId = data.value("id").toInt();
        emit insertImageFinished(requestId, code == 0, imageId, message);
        return;
    }

    if (type == "DELETE_IMAGES_RESP") {
        emit deleteImagesFinished(requestId, code == 0, message);
        return;
    }

    if (type == "QUERY_LOG_LIST_RESP") {
        QJsonObject data = object.value("data").toObject();
        QJsonArray list = data.value("list").toArray();
        int totalCount = data.value("total_count").toInt();

        emit logListFinished(requestId, code == 0, list, totalCount, message);
        return;
    }
}

qint64 ClientApi::nextRequestId()
{
    return m_nextRequestId++;
}

QJsonObject ClientApi::makeRequest(const QString &type, const QJsonObject &data)
{
    const qint64 requestId = nextRequestId();

    QJsonObject request;
    request["type"] = type;
    request["request_id"] = requestId;
    request["data"] = data;

    return request;
}
