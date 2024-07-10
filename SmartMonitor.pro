QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    video_widget.cpp

HEADERS += \
    video_widget.h

FORMS +=

# translation config
TRANSLATIONS += \
    SmartMonitor_zh_CN.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# import FFmpeg
FFMPEG_HOME=D:\sdk\ffmpeg-7.0.1-full_build-shared
INCLUDEPATH += $$FFMPEG_HOME/include
LIBS += -L$$FFMPEG_HOME/lib \
        -lavcodec \
        -lavdevice \
        -lavfilter \
        -lavformat \
        -lavutil \
        -lpostproc \
        -lswresample \
        -lswscale\

# import OpenCV
INCLUDEPATH += D:\sdk\OpenCV-MinGW-Build-OpenCV-3.4.8-x64\include
LIBS += D:\sdk\OpenCV-MinGW-Build-OpenCV-3.4.8-x64\x64\mingw\bin\libopencv_*.dll

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    .gitignore

