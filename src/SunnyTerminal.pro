
QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SunnyTerminal
TEMPLATE = app

DEFINES += "APPNAME=\"\\\"SunnyTerminal v1.0.0\\\"\""

SOURCES += main.cpp \
           mainwindow.cpp \
           receiver.cpp \
           terminal.cpp \
           core.cpp \
           title.cpp

HEADERS += mainwindow.h \
           receiver.h \
           terminal.h \
           Pty/ptylib.h \
           core.h \
           title.h

FORMS   += mainwindow.ui

win32 {
SOURCES += Pty/ptycom.c
}

unix {
message("SunnyTerminal Build Settings" )
SOURCES += Pty/ptylib.c

equals(CROSS_COMPILE,"Enabled") {

DEFINES += STATIC_BUILD
CONFIG  += static
QMAKE_LFLAGS += -static
#QT5PATH is provided from outside...
LIBS    += -L$$QT5PATH/plugins/imageformats
LIBS    += -L$$QT5PATH/plugins/platforms
LIBS    += -L$$QT5PATH/plugins/generic
QTPLUGIN += qlinuxfb qevdevkeyboardplugin qevdevmouseplugin
message( $$QT5PATH )
message( $$LIBS )

} else {

LIBS += -lutil

}
}

#--------------------------------------------------------------------------------------------------
#- copying fonts to build directory ---------------------------------------------------------------
#--------------------------------------------------------------------------------------------------

CONFIG(debug, debug|release) {
# Do not copy fonts. Assume it is located in system fonts
} else {
win32: copydata.commands = $(COPY_DIR) $$shell_path($$PWD/fonts) $$shell_path($$OUT_PWD/release/fonts)
unix:  copydata.commands = $(COPY_DIR) $$shell_path($$PWD/fonts) $$shell_path($$OUT_PWD)
first.depends = $(first) copydata
export(first.depends)
export(copydata.commands)
QMAKE_EXTRA_TARGETS += first copydata
}
