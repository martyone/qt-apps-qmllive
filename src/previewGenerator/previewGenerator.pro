include(../../qmllive.pri)

TEMPLATE = app
TARGET = previewGenerator
DESTDIR = $$BUILD_DIR/libexec/qmllive

# install rules
macx*: CONFIG -= app_bundle
target.path = $$PREFIX/libexec/qmllive
INSTALLS += target

QT = gui core quick

SOURCES += \
    main.cpp
