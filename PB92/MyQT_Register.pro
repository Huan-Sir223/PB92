#-------------------------------------------------
#
# Project created by QtCreator 2018-07-10T00:09:38
#
#-------------------------------------------------

QT       += core gui

QT       += serialport

QT       +=sql
QT       += multimedia
QT       += concurrent
QT += network

UI_DIR=./UI
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MyQT_Register
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

#INCLUDEPATH += D:\opencv3.4.3\opencv\build\include \
#               D:\opencv3.4.3\opencv\build\include\opencv \
#               D:\opencv3.4.3\opencv\build\include\opencv2 \

#LIBS += D:\opencv3.4.3\buildOpencv\install\x86\mingw\bin\libopencv_core343.dll \
#    D:\opencv3.4.3\buildOpencv\install\x86\mingw\bin\libopencv_highgui343.dll \
#    D:\opencv3.4.3\buildOpencv\install\x86\mingw\bin\libopencv_imgcodecs343.dll \
#    D:\opencv3.4.3\buildOpencv\install\x86\mingw\bin\libopencv_imgproc343.dll \
#    D:\opencv3.4.3\buildOpencv\install\x86\mingw\bin\libopencv_features2d343.dll \
#    D:\opencv3.4.3\buildOpencv\install\x86\mingw\bin\libopencv_calib3d343.dll

#LIBS += -LD:\opencv3.4.3\buildOpencv\install\x86\mingw\lib\libopencv_*.a
#LIBS += -LD:\opencv3.4.3\buildOpencv\bin\libopencv_*.dll

#LIBS += D:\opencv3.4.3\buildOpencv\install\x86\mingw\bin\libopencv_shape343.dll
#LIBS += D:\opencv3.4.3\buildOpencv\install\x86\mingw\bin\libopencv_videoio343.dll
INCLUDEPATH += D:\QT\opencv\opencv-build\install\include
LIBS += D:\QT\opencv\opencv-build\lib\libopencv_*.a


SOURCES += \
    felfouttime.cpp \
        main.cpp \
        mainwindow.cpp \
    myqcombobox.cpp \
    mytcpserver.cpp \
    parsedata.cpp \
    qextserialbase.cpp \
    qextserialport.cpp \
    tcpthread.cpp \
    win_qextserialport.cpp \
    netdownload.cpp \
    fcamera.cpp \
    playimage.cpp \
    scamera.cpp

HEADERS += \
    felfouttime.h \
        mainwindow.h \
    myqcombobox.h \
    mytcpserver.h \
    parsedata.h \
    qextserialbase.h \
    qextserialport.h \
    tcpthread.h \
    win_qextserialport.h \
    netdownload.h \
    fcamera.h \
    fcamera.h \
    playimage.h \
    scamera.h

FORMS += \
    felfouttime.ui \
        mainwindow.ui \
    netdownload.ui \
    fcamera.ui \
    fcamera.ui \
    scamera.ui

LIBS += -lWs2_32

