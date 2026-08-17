#ifndef DBCONNECTIONPOOL_H
#define DBCONNECTIONPOOL_H

#include <QHash>
#include <QMutex>
#include <QQueue>
#include <QSet>
#include <QSqlDatabase>
#include <QString>
#include <QWaitCondition>

class DbConnectionPool;

class DbConnectionGuard
{
public:
    DbConnectionGuard();
    ~DbConnectionGuard();

    DbConnectionGuard(const DbConnectionGuard &) = delete;
    DbConnectionGuard &operator=(const DbConnectionGuard &) = delete;

    DbConnectionGuard(DbConnectionGuard &&other) noexcept;
    DbConnectionGuard &operator=(DbConnectionGuard &&other) noexcept;

    bool isValid() const;
    QSqlDatabase database() const;

private:
    friend class DbConnectionPool;

    DbConnectionGuard(DbConnectionPool *pool,
                      const QString &connectionName,
                      const QString &threadKey,
                      const QSqlDatabase &database);

    void release();

    DbConnectionPool *m_pool;
    QString m_connectionName;
    QString m_threadKey;
    QSqlDatabase m_database;
};

class DbConnectionPool
{
public:
    static DbConnectionPool *instance();

    DbConnectionGuard acquire(int timeoutMs = 3000);
    void release(const QString &connectionName, const QString &threadKey);
    void closeAll();

    DbConnectionPool(const DbConnectionPool &) = delete;
    DbConnectionPool &operator=(const DbConnectionPool &) = delete;

private:
    DbConnectionPool();
    ~DbConnectionPool();

    QString currentThreadKey() const;
    QSqlDatabase createConnection(const QString &connectionName);

    mutable QMutex m_mutex;
    QWaitCondition m_waitCondition;
    QHash<QString, QQueue<QString>> m_idleConnections;
    QHash<QString, int> m_createdCountByThread;
    QSet<QString> m_allConnectionNames;
    int m_maxConnectionsPerThread;
};

#endif // DBCONNECTIONPOOL_H
