TARGET = qmlliveruntime
DESTDIR = $$BUILD_DIR/bin

QT *= widgets quick
osx: CONFIG -= app_bundle

SOURCES += main.cpp

win32: RC_FILE = ../../icons/appicon.rc

include(../widgets/widgets.pri)
include(../lib.pri)

RESOURCES += \
    qml.qrc
