TARGET = kobo

TEMPLATE = lib
CONFIG += plugin

DEFINES += QT_NO_FOREACH
CONFIG += relative_qt_rpath  # Qt's plugins should be relocatable


QT +=  widgets \
    core-private gui-private \
    service_support-private eventdispatcher_support-private \
    fontdatabase_support-private fb_support-private devicediscovery_support-private testlib

#qtHaveModule(input_support-private): \
#    QT += input_support-private

SOURCES = main.cpp \
          eink.cpp \
          einkrefreshthread.cpp \
          kobobuttonintegration.cpp \
          kobodevicedescriptor.cpp \
          kobofbscreen.cpp \
          koboplatformadditions.cpp \
          koboplatformintegration.cpp \
          kobowifimanager.cpp \
          qevdevtouchmanager.cpp \
          qevdevtouchhandler.cpp \ 
          qtouchoutputmapping.cpp \
          qevdevutil.cpp

HEADERS = devicehandlerlist_p.h \
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
          partialrefreshmode.h \
          qevdevtouchfilter_p.h \
          qevdevtouchhandler_p.h \
          qevdevtouchmanager_p.h \
          qevdevutil_p.h \
          qtouchoutputmapping_p.h


OTHER_FILES += \

target.path = /mnt/onboard/.adds/qt-linux-5.15.2-kobo/plugins/platforms
INSTALLS += target

DISTFILES += \
    koboplatformplugin.json

RESOURCES += \
    Resources.qrc
