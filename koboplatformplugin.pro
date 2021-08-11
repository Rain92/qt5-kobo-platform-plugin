TARGET = kobo

TEMPLATE = lib
CONFIG += plugin

DEFINES += QT_NO_FOREACH
CONFIG += relative_qt_rpath  # Qt's plugins should be relocatable


QT +=  widgets \
    core-private gui-private \
    service_support-private eventdispatcher_support-private \
    fontdatabase_support-private fb_support-private devicediscovery_support-private testlib



LIBS += -L/home/andreas/Desktop/qtproject/FBInk/Release -lfbink


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
          mxcfb-kobo.h \
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
