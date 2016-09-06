include(../../qmllive.pri)

TARGET = qmlliveruntime
DESTDIR = $$BUILD_DIR/bin

QT *= widgets quick
macos: CONFIG -= app_bundle

SOURCES += main.cpp

win32: RC_FILE = ../../icons/appicon.rc

include(../widgets/widgets.pri)

static-link-runtime: include(../src.pri)
else: include(../lib.pri)

RESOURCES += \
    qml.qrc

target.path = $$PREFIX/bin
INSTALLS += target
