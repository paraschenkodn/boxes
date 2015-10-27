QT += opengl widgets

contains(QT_CONFIG, opengles.|angle|dynamicgl):error("This example requires Qt to be configured with -opengl desktop")

HEADERS += \
           scene.h \
    oglw.h \
    dialogboxes.h
SOURCES += \
           main.cpp \
           scene.cpp \
    dialogboxes.cpp \
    oglw.cpp

RESOURCES += boxes.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/graphicsview/boxes
INSTALLS += target

wince*: {
    DEPLOYMENT_PLUGIN += qjpeg
}
