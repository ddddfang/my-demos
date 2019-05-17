TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

LIBS += -L/usr/lib/x86_64-linux-gnu -lprotobuf
INCLUDEPATH +=/usr/include

SOURCES += main.cpp \
    proto/fang.pb.cc

DISTFILES += \
    proto/fang.proto

HEADERS += \
    proto/fang.pb.h
