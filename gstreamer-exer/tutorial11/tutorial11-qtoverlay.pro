QT += core gui widgets
TARGET = qtoverlay

INCLUDEPATH += /usr/include/glib-2.0
INCLUDEPATH += /usr/lib/x86_64-linux-gnu/glib-2.0/include
INCLUDEPATH += /usr/local/include/gstreamer-1.0
LIBS += -lgstreamer-1.0 -lgobject-2.0 -lglib-2.0 -lgstvideo-1.0

SOURCES += tutorial11-qtoverlay.cpp
HEADERS += tutorial11-qtoverlay.h
