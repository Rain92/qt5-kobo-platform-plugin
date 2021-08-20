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

INCLUDEPATH += $$PWD/src

# FBINK
DEFINES += FBINK_FOR_KOBO
FBInkBuildEvent.input = $$PWD/FBink/*.c $$PWD/FBink/*.h
PHONY_DEPS = .
FBInkBuildEvent.input = PHONY_DEPS
FBInkBuildEvent.output = FBInk
FBInkBuildEvent.clean_commands = make -C $$PWD/FBInk clean

FBInkBuildEvent.name = building FBInk
FBInkBuildEvent.CONFIG += no_link target_predeps
QMAKE_EXTRA_COMPILERS += FBInkBuildEvent

INCLUDEPATH += $$PWD/FBInk $$PWD/FBInk/libi2c-staged/include
LIBS += -L$$PWD/FBInk/libi2c-staged/lib/ -l:libi2c.a

CONFIG(debug, debug|release) {
    FBInkBuildEvent.commands = CROSS_TC=$$CROSS_TC MINIMAL=1 DEBUG=1 KOBO=true make -C $$PWD/FBInk pic
    LIBS += -L$$PWD/FBInk/Debug -l:libfbink.a
}

CONFIG(release, debug|release) {
FBInkBuildEvent.commands = CROSS_TC=$$CROSS_TC MINIMAL=1 KOBO=true make -C $$PWD/FBInk pic
    LIBS += -L$$PWD/FBInk/Release -l:libfbink.a
}


SOURCES = src/main.cpp \
          src/dither.cpp \
          src/kobobuttonintegration.cpp \
          src/kobodevicedescriptor.cpp \
          src/kobofbscreen.cpp \
          src/koboplatformadditions.cpp \
          src/koboplatformintegration.cpp \
          src/kobowifimanager.cpp \
          src/qevdevtouchmanager.cpp \
          src/qevdevtouchhandler.cpp

HEADERS = \
          src/dither.h \
          src/einkenums.h \
          src/kobobuttonintegration.h \
          src/kobodevicedescriptor.h \
          src/kobofbscreen.h \
          src/koboplatformadditions.h \
          src/koboplatformfunctions.h \
          src/koboplatformintegration.h \
          src/kobowifimanager.h \
          src/qevdevtouchfilter_p.h \
          src/qevdevtouchhandler_p.h \
          src/qevdevtouchmanager_p.h


OTHER_FILES += \

target.path =$$QTDIR/plugins/platforms
INSTALLS += target

DISTFILES += \
    koboplatformplugin.json

RESOURCES += \
    Resources.qrc

