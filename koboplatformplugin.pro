TARGET = kobo

TEMPLATE = lib
CONFIG += plugin

DEFINES += QT_NO_FOREACH
CONFIG += relative_qt_rpath  # Qt's plugins should be relocatable


QT += \
    core-private gui-private \
    service_support-private eventdispatcher_support-private \
    fontdatabase_support-private fb_support-private

qtHaveModule(input_support-private): \
    QT += input_support-private

SOURCES = main.cpp \
          einkrefreshthread.cpp \
          kobobuttonintegration.cpp \
          kobofbscreen.cpp \
          koboplatformadditions.cpp \
          koboplatformintegration.cpp \
          kobowifimanager.cpp

HEADERS = \
          einkrefreshthread.h \
          eink.h \
          kobobuttonintegration.h \
          kobodevicedescriptor.h \
          kobofbscreen.h \
          kobokey.h \
          koboplatformadditions.h \
          koboplatformfunctions.h \
          koboplatformintegration.h \
          kobowifimanager.h \
          mxcfb-kobo.h \
          partialrefreshmode.h


OTHER_FILES += \

target.path = /mnt/onboard/.adds/qt-5.14.1-kobo/plugins/platforms
INSTALLS += target

DISTFILES += \
    koboplatformplugin.json

RESOURCES += \
    Resources.qrc
