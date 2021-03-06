QT += dbus network

include(../common-install.pri)

TEMPLATE = app
TARGET = sensordataflow-test

CONFIG += testcase

HEADERS += dataflowtests.h
SOURCES += dataflowtests.cpp

INCLUDEPATH += ../../include \
    ../../chains \
    ../../core \
    ../../datatypes \
    ../../chains \
    ../../filters \
    ../../filters/coordinatealignfilter \
    ../../adaptors \
    ../..

QMAKE_LIBDIR_FLAGS += -L../../builddir/datatypes -L../../datatypes/
QMAKE_LIBDIR_FLAGS += -L../../builddir/core -L../../core/
QMAKE_RPATHDIR += /usr/lib/sensord

equals(QT_MAJOR_VERSION, 4):{
    QMAKE_LIBDIR_FLAGS += -lsensorfw
}
equals(QT_MAJOR_VERSION, 5):{
    QMAKE_LIBDIR_FLAGS += -lsensorfw-qt5
}
