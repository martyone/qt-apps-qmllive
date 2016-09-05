INCLUDEPATH += $${PWD}
LIBS += -L$${BUILD_DIR}/lib -lqmllive
# This can be simply eliminated by passing '-after QMAKE_RPATHDIR=' to qmake
QMAKE_RPATHDIR += $${BUILD_DIR}/lib
