include(../../qmllive.pri)

TEMPLATE = app
TARGET = previewGenerator
DESTDIR = $$BUILD_DIR/libexec/qmllive

# install rules
macos: CONFIG -= app_bundle
target.path = $$PREFIX/libexec/qmllive
INSTALLS += target

QT = gui core quick widgets

SOURCES += \
    main.cpp
