QT += widgets sql multimedia multimediawidgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/main.cpp \
    tool/src/Detect.cpp \
    tool/src/YOLO.cpp \
    src/view/exceptionwidget.cpp \
    src/model/exception.cpp \
    src/model/exceptionmodel.cpp \
    src/model/image.cpp \
    src/model/imagemodel.cpp \
    src/model/logmodel.cpp \
    src/service/detectask.cpp \
    src/util/appsettings.cpp \
    src/model/usermodel.cpp \
    src/model/video.cpp \
    src/model/videomodel.cpp \
    src/service/monitortask.cpp \
    src/service/recordtask.cpp \
    src/service/userservice.cpp \
    src/util/dbconnectionpool.cpp \
    src/util/dbconn.cpp \
    src/util/winmannager.cpp \
    src/view/loginwidget.cpp \
    src/view/logwidget.cpp \
    src/view/mainwidget.cpp \
    src/view/monitorwidget.cpp \
    src/view/registerwidget.cpp \
    src/view/detailwidget.cpp \
    src/view/reviewwidget.cpp \
    src/view/selectrl.cpp \
    src/view/settingwidget.cpp \
    src/view/verifycodelabel.cpp


HEADERS += \
    src/view/exceptionwidget.h \
    src/model/exception.h \
    src/model/exceptionmodel.h \
    src/model/image.h \
    src/model/imagemodel.h \
    src/model/logmodel.h \
    src/service/detectask.h \
    src/util/appsettings.h \
    src/model/usermodel.h \
    src/model/video.h \
    src/model/videomodel.h \
    src/service/monitortask.h \
    src/service/recordtask.h \
    src/service/userservice.h \
    src/util/dbconnectionpool.h \
    src/util/dbconn.h \
    src/util/winmannager.h \
    src/view/loginwidget.h \
    src/view/logwidget.h \
    src/view/mainwidget.h \
    src/view/monitorwidget.h \
    src/view/registerwidget.h \
    src/view/detailwidget.h \
    src/view/reviewwidget.h \
    src/view/selectrl.h \
    src/view/settingwidget.h \
    src/view/verifycodelabel.h\
    tool/include/Detect.h\
    tool/include/YOLO.h


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

//配置opencv路径
OPENCV_ROOT=D:/opencv
#OPENCV头文件
INCLUDEPATH+=$${OPENCV_ROOT}/include
INCLUDEPATH += $$PWD/src
INCLUDEPATH += $$PWD/tool/include
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
TARGET=VideoPlayer

