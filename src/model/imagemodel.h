#ifndef IMAGEMODEL_H
#define IMAGEMODEL_H

#include "image.h"

#include <QDate>

class ImageModel
{
public:
    ImageModel();

    bool insertImage(const Image &image);
    bool deleteImagesByIds(const QList<int> &imageIds);
    QList<Image> findImagesByIds(const QList<int> &imageIds);
    int countByDate(const QDateTime &startTime,
                    const QDateTime &endTime,
                    int channelId,
                    bool anomalyOnly) const;
    QList<Image> queryPageByDate(const QDateTime &startTime,
                                 const QDateTime &endTime,
                                 int channelId,
                                 bool anomalyOnly,
                                 int offset,
                                 int limit) const;
    QDate latestDate(bool anomalyOnly) const;
};

#endif // IMAGEMODEL_H
