!CONFIG(skip-bench): requires(qtHaveModule(widgets))
requires(!winrt)

load(configure)
load(config-output)
include(qmllive.pri)

!minQtVersion(5, 4, 0):error("You need at least Qt 5.4.0 to build this application")

TEMPLATE = subdirs
CONFIG += ordered

android|ios {
    message("Note: the bench, examples and shared library will not be built on this platform")
    CONFIG += skip-bench skip-examples static-link-runtime
}

SUBDIRS += src
!CONFIG(skip-tests): SUBDIRS += tests
!CONFIG(skip-examples): SUBDIRS += examples

OTHER_FILES += \
    README.md \
    INSTALL.md \
    CONTRIBUTORS.md

include(doc/doc.pri)
