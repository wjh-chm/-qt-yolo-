#ifndef LOGMODEL_H
#define LOGMODEL_H

#include <QList>
#include <QString>

class LogModel
{
public:
    enum class Mode {
        Operation,
        Exception
    };

    struct Record {
        int id = 0;
        QString operation;
        int operatorId;
        QString operateTime;
    };

    LogModel();

    int count(Mode mode) const;
    QList<Record> queryPage(Mode mode, int offset, int limit) const;
};

#endif // LOGMODEL_H
