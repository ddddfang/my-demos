#-------------------------------------------------
#
# Project created by QtCreator 2018-12-01T18:23:50
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = test_widgt
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

FFMPEG_ROOT = /home/fang/ffmpeg
INCLUDEPATH +=  $$FFMPEG_ROOT/include
LIBS += -L$$FFMPEG_ROOT/lib -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lswresample -lswscale -lz -lm

INCLUDEPATH += /usr/local/include
LIBS += -L/usr/local/lib -lSDL2

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    videoplayer.cpp

HEADERS += \
        mainwindow.h \
    videoplayer.h

FORMS += \
        mainwindow.ui
