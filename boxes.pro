QT += opengl widgets

contains(QT_CONFIG, opengles.|angle|dynamicgl):error("This example requires Qt to be configured with -opengl desktop")

HEADERS += \
           glbuffers.h \
           glextensions.h \
           gltrianglemesh.h \
           qtbox.h \
           scene.h \
    dialogboxes.h
SOURCES += \
           glbuffers.cpp \
           glextensions.cpp \
           main.cpp \
           qtbox.cpp \
           scene.cpp \
    dialogboxes.cpp

RESOURCES += boxes.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/graphicsview/boxes
INSTALLS += target

wince*: {
    DEPLOYMENT_PLUGIN += qjpeg
}
