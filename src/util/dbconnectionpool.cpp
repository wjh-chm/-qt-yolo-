#include "dbconnectionpool.h"

#include "appsettings.h"

#include <QDebug>
#include <QMutexLocker>
#include <QSqlError>
#include <QThread>
#include <QStringList>

#include <utility>

DbConnectionGuard::DbConnectionGuard()
    : m_pool(nullptr)
{
}

DbConnectionGuard::DbConnectionGuard(DbConnectionPool *pool,
                                     const QString &connectionName,
                                     const QString &threadKey,
                                     const QSqlDatabase &database)
    : m_pool(pool),
      m_connectionName(connectionName),
      m_threadKey(threadKey),
      m_database(database)
{
}

DbConnectionGuard::~DbConnectionGuard()
{
    release();
}

DbConnectionGuard::DbConnectionGuard(DbConnectionGuard &&other) noexcept
    : m_pool(other.m_pool),
      m_connectionName(other.m_connectionName),
      m_threadKey(other.m_threadKey),
      m_database(other.m_database)
{
    other.m_pool = nullptr;
    other.m_connectionName.clear();
    other.m_threadKey.clear();
    other.m_database = QSqlDatabase();
}

DbConnectionGuard &DbConnectionGuard::operator=(DbConnectionGuard &&other) noexcept
{
    if (this != &other) {
        release();
        m_pool = other.m_pool;
        m_connectionName = other.m_connectionName;
        m_threadKey = other.m_threadKey;
        m_database = other.m_database;

        other.m_pool = nullptr;
        other.m_connectionName.clear();
        other.m_threadKey.clear();
        other.m_database = QSqlDatabase();
    }

    return *this;
}

bool DbConnectionGuard::isValid() const
{
    return m_pool != nullptr && m_database.isValid() && m_database.isOpen();
}

QSqlDatabase DbConnectionGuard::database() const
{
    return m_database;
}

void DbConnectionGuard::release()
{
    if (m_pool == nullptr || m_connectionName.isEmpty()) {
        return;
    }

    DbConnectionPool *pool = m_pool;
    const QString connectionName = m_connectionName;
    const QString threadKey = m_threadKey;

    m_pool = nullptr;
    m_connectionName.clear();
    m_threadKey.clear();
    m_database = QSqlDatabase();

    pool->release(connectionName, threadKey);
}

DbConnectionPool *DbConnectionPool::instance()
{
    static DbConnectionPool pool;
    return &pool;
}

DbConnectionPool::DbConnectionPool()
    : m_maxConnectionsPerThread(4)
{
}

DbConnectionPool::~DbConnectionPool()
{
    closeAll();
}

DbConnectionGuard DbConnectionPool::acquire(int timeoutMs)
{
    const QString threadKey = currentThreadKey();
    QString connectionName;

    {
        QMutexLocker locker(&m_mutex);
        while (true) {
            QQueue<QString> &idleQueue = m_idleConnections[threadKey];
            if (!idleQueue.isEmpty()) {
                connectionName = idleQueue.dequeue();
                break;
            }

            const int createdCount = m_createdCountByThread.value(threadKey, 0);
            if (createdCount < m_maxConnectionsPerThread) {
                connectionName = QString("smart_monitor_pool_%1_%2").arg(threadKey).arg(createdCount + 1);
                m_createdCountByThread[threadKey] = createdCount + 1;
                m_allConnectionNames.insert(connectionName);
                break;
            }

            if (!m_waitCondition.wait(&m_mutex, timeoutMs)) {
                qDebug() << "database pool acquire timeout";
                return DbConnectionGuard();
            }
        }
    }

    QSqlDatabase db;
    if (QSqlDatabase::contains(connectionName)) {
        db = QSqlDatabase::database(connectionName, false);
    } else {
        db = createConnection(connectionName);
    }

    if (!db.isOpen() && !db.open()) {
        qDebug() << "database pool open failed:" << db.lastError().text();
        release(connectionName, threadKey);
        return DbConnectionGuard();
    }

    return DbConnectionGuard(this, connectionName, threadKey, db);
}

void DbConnectionPool::release(const QString &connectionName, const QString &threadKey)
{
    if (connectionName.isEmpty()) {
        return;
    }

    QMutexLocker locker(&m_mutex);
    if (!m_allConnectionNames.contains(connectionName)) {
        return;
    }

    m_idleConnections[threadKey].enqueue(connectionName);
    m_waitCondition.wakeOne();
}

void DbConnectionPool::closeAll()
{
    QStringList connectionNames;
    {
        QMutexLocker locker(&m_mutex);
        for (const QString &connectionName : std::as_const(m_allConnectionNames)) {
            connectionNames.append(connectionName);
        }
        m_idleConnections.clear();
        m_createdCountByThread.clear();
        m_allConnectionNames.clear();
    }

    for (const QString &connectionName : connectionNames) {
        if (!QSqlDatabase::contains(connectionName)) {
            continue;
        }

        {
            QSqlDatabase db = QSqlDatabase::database(connectionName, false);
            if (db.isOpen()) {
                db.close();
            }
        }
        QSqlDatabase::removeDatabase(connectionName);
    }
}

QString DbConnectionPool::currentThreadKey() const
{
    const quintptr threadId = reinterpret_cast<quintptr>(QThread::currentThreadId());
    return QString::number(threadId, 16);
}

QSqlDatabase DbConnectionPool::createConnection(const QString &connectionName)
{
    const DatabaseSettings settings = AppSettings::loadDatabaseSettings();

    QSqlDatabase db = QSqlDatabase::addDatabase(settings.driver, connectionName);
    db.setHostName(settings.host);
    db.setPort(settings.port);
    db.setUserName(settings.userName);
    db.setPassword(settings.password);
    db.setDatabaseName(settings.databaseName);
    return db;
}
