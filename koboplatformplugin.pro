TARGET = kobo

QTDIR = /mnt/onboard/.adds/qt-linux-5.15-kde-kobo
CROSS_TC = arm-kobo-linux-gnueabihf

TEMPLATE = lib
CONFIG += plugin

DEFINES += QT_NO_FOREACH

QMAKE_RPATHDIR += ../../lib

QT +=  widgets \
    core-private gui-private \
    service_support-private eventdispatcher_support-private \
    fontdatabase_support-private fb_support-private devicediscovery_support-private testlib


# FBINK
DEFINES += FBINK_FOR_KOBO
FBInkBuildEvent.input = ./FBink/*.c ./FBink/*.h
PHONY_DEPS = .
FBInkBuildEvent.input = PHONY_DEPS
FBInkBuildEvent.output = FBInk
FBInkBuildEvent.clean_commands = make -C $$PWD/FBInk clean distclean

FBInkBuildEvent.name = building FBInk
FBInkBuildEvent.CONFIG += no_link target_predeps
QMAKE_EXTRA_COMPILERS += FBInkBuildEvent

INCLUDEPATH += $$PWD/FBInk $$PWD/FBInk/ $$PWD/FBInk/libi2c-staged/include/
LIBS += -L$$PWD/FBInk/libi2c-staged/lib/ -l:libi2c.a

CONFIG(debug, debug|release) {
    FBInkBuildEvent.commands = CROSS_TC=$$CROSS_TC MINIMAL=1 DEBUG=1 KOBO=true make -C $$PWD/FBInk libi2c.built pic
    LIBS += -L$$PWD/FBInk/Debug -l:libfbink.a
}

CONFIG(release, debug|release) {
FBInkBuildEvent.commands = CROSS_TC=$$CROSS_TC MINIMAL=1 KOBO=true make -C $$PWD/FBInk libi2c.built pic
    LIBS += -L$$PWD/FBInk/Debug -l:libfbink.a
}


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

target.path =$$QTDIR/plugins/platforms
INSTALLS += target

DISTFILES += \
    koboplatformplugin.json

RESOURCES += \
    Resources.qrc

