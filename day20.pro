QT += widgets sql multimedia multimediawidgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    service/detectask.cpp \
    util/appsettings.cpp \
    model/usermodel.cpp \
    model/video.cpp \
    model/videomodel.cpp \
    service/monitortask.cpp \
    service/recordtask.cpp \
    service/userservice.cpp \
    util/dbconn.cpp \
    util/winmannager.cpp \
    view/loginwidget.cpp \
    view/mainwidget.cpp \
    view/monitorwidget.cpp \
    view/registerwidget.cpp \
    view/reviewwidget.cpp \
    view/selectrl.cpp \
    view/settingwidget.cpp \
    view/verifycodelabel.cpp


HEADERS += \
    service/detectask.h \
    util/appsettings.h \
    model/usermodel.h \
    model/video.h \
    model/videomodel.h \
    service/monitortask.h \
    service/recordtask.h \
    service/userservice.h \
    util/dbconn.h \
    util/winmannager.h \
    view/loginwidget.h \
    view/mainwidget.h \
    view/monitorwidget.h \
    view/registerwidget.h \
    view/reviewwidget.h \
    view/selectrl.h \
    view/settingwidget.h \
    view/verifycodelabel.h


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

//配置opencv路径
OPENCV_ROOT=D:/opencv
#OPENCV头文件
INCLUDEPATH+=$${OPENCV_ROOT}/include
#opencv库文件
LIBS+=-L$${OPENCV_ROOT}/x64/mingw/lib


#核心模块(内存管理、数据结构
LIBS += -llibopencv_core4100
#图像处理(滤波、缩放等)
LIBS += -llibopencv_imgproc4100
#视频I/O(调用FFmpeg读写视频)
LIBS += -llibopencv_videoio4100
#简单GUI显示(如imshow)
LIBS += -llibopencv_highgui4100
LIBS += -llibopencv_imgcodecs4100 #图像编解码(读写图片)
LIBS += -llibopencv_features2d4100#特征检测(如 SIFT/SURF)
LIBS += -llibopencv_calib3d4100 #相机标定、3D重建
LIBS += -llibopencv_objdetect4100 #目标检测(如Haar 级联
LIBS += -llibopencv_dnn4100
DESTDIR=$$PWD/bin

RESOURCES += \
    resources.qrc
