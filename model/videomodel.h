#ifndef VIDEOMODEL_H
#define VIDEOMODEL_H

#include "video.h"

#include <QFileInfo>

// 视频记录写入数据库的数据访问封装。
class VideoModel
{
public:
    VideoModel();

    // 将一条完成录制的视频记录写入数据库。
    bool insertVideo(const Video &video);
};

#endif // VIDEOMODEL_H
