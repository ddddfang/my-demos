
TEMPLATE = app
TARGET = tutorial
INCLUDEPATH += .
INCLUDEPATH += /home/fang/Qt5.12.0/5.12.0/gcc_64/include
INCLUDEPATH += /home/fang/Qt5.12.0/5.12.0/gcc_64/include/QtCore
INCLUDEPATH += -I/usr/include/alsa

LIBS += -L/home/fang/Qt5.12.0/5.12.0/gcc_64/lib -lQt5Core
LIBS += -lasound

CONFIG += c++11
#QMAKE_CXXFLAGS += -std=c++11

DEFINES += QT_DEPRECATED_WARNINGS

# Input
HEADERS += readerThread.h alsaCaptureAudio.h
SOURCES += test_alsa.cpp readerThread.cpp alsaCaptureAudio.cpp

QT += widgets core
QT -= gui

