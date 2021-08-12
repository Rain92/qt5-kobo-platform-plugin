TARGET = kobo

TEMPLATE = lib
CONFIG += plugin


DEFINES += FBINK_FOR_KOBO

DEFINES += QT_NO_FOREACH

QMAKE_RPATHDIR += ../../lib

QT +=  widgets \
    core-private gui-private \
    service_support-private eventdispatcher_support-private \
    fontdatabase_support-private fb_support-private devicediscovery_support-private testlib

LIBS += -l:libi2c.a -L/home/andreas/Desktop/qtproject/FBInk/Debug -l:libfbink.a


SOURCES = main.cpp \
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
          kobobuttonintegration.h \
          kobodevicedescriptor.h \
          kobofbscreen.h \
          kobokey.h \
          koboplatformadditions.h \
          koboplatformfunctions.h \
          koboplatformintegration.h \
          kobowifimanager.h \
          qevdevtouchfilter_p.h \
          qevdevtouchhandler_p.h \
          qevdevtouchmanager_p.h \
          qevdevutil_p.h \
          qtouchoutputmapping_p.h \
          refreshmode.h


OTHER_FILES += \

target.path = /mnt/onboard/.adds/qt-linux-5.15-kde-kobo/plugins/platforms
INSTALLS += target

DISTFILES += \
    koboplatformplugin.json

RESOURCES += \
    Resources.qrc
