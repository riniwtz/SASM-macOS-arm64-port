#-------------------------------------------------
#
# Project created by QtCreator 2013-03-22T02:19:57
#
#-------------------------------------------------

QT       += core gui
CONFIG += c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = sasm
TEMPLATE = app

isEmpty(PREFIX) {
 PREFIX = /usr
}

BINDIR = $$PREFIX/bin
DATADIR = $$PREFIX/share

bsd {
    binfile.files += sasm
    binfile.files += BSD/bin/*
    binfile.path = $$BINDIR
    data.files += BSD/share/sasm/*
    data.path = $$DATADIR/sasm/
    shortcutfiles.files += BSD/share/applications/sasm.desktop
    shortcutfiles.path = $$DATADIR/applications/
    docfiles.files += BSD/share/doc/sasm/changelog.gz
    docfiles.files += BSD/share/doc/sasm/copyright
    docfiles.path = $$DATADIR/doc/sasm/
} else:!macx {
    binfile.files += sasm
    binfile.files += Linux/bin/*
    binfile.path = $$BINDIR
    data.files += Linux/share/sasm/*
    data.path = $$DATADIR/sasm/
    shortcutfiles.files += Linux/share/applications/sasm.desktop
    shortcutfiles.path = $$DATADIR/applications/
    docfiles.files += Linux/share/doc/sasm/changelog.gz
    docfiles.files += Linux/share/doc/sasm/copyright
    docfiles.path = $$DATADIR/doc/sasm/
}

INSTALLS += binfile
INSTALLS += data
INSTALLS += shortcutfiles
INSTALLS += docfiles

include(singleapplication/qtsingleapplication.pri)

SOURCES += main.cpp\
        mainwindow.cpp \
    tab.cpp \
    highlighter.cpp \
    codeeditor.cpp \
    debugger.cpp \
    finddialog.cpp \
    ruqtextedit.cpp \
    ruqplaintextedit.cpp \
    getstartedwidget.cpp \
    debugtablewidget.cpp \
    watchsettingswidget.cpp \
    debuganycommandwidget.cpp \
    assembler.cpp \
    nasm.cpp \
    gas.cpp \
    common.cpp \
    fasm.cpp \
    signallocker.cpp \
    masm.cpp \
    gccbasedassembler.cpp

HEADERS  += mainwindow.h \
    tab.h \
    highlighter.h \
    codeeditor.h \
    debugger.h \
    finddialog.h \
    ruqtextedit.h \
    ruqplaintextedit.h \
    getstartedwidget.h \
    debugtablewidget.h \
    watchsettingswidget.h \
    debuganycommandwidget.h \
    assembler.h \
    nasm.h \
    gas.h \
    common.h \
    fasm.h \
    signallocker.h \
    masm.h \
    gccbasedassembler.h

FORMS += settings.ui

RESOURCES += \
    Languages.qrc \
    Images.qrc \
    Help.qrc

win32 {
 RC_FILE = icon.rc
}

macx {
 QMAKE_MACOSX_DEPLOYMENT_TARGET = 11.0
 ICON = sasm.icns
 QMAKE_INFO_PLIST = Info.plist
 SOURCES -= debugger.cpp
 SOURCES += macos/debugger.cpp macos/platform.cpp
 HEADERS += macos/platform.h
 data.files = $$PWD/Linux/share/sasm/keys.ini
 data.path = Contents/Resources/sasm
 macdata.files = $$PWD/macos/lldb_bridge.py
 macdata.path = Contents/Resources/sasm
 examples.files = $$files($$PWD/macos/Projects/*)
 examples.path = Contents/Resources/sasm/Projects
 macros.files = $$PWD/Linux/share/sasm/NASM/macro.c
 macros.path = Contents/Resources/sasm/NASM
 macincludes.files = $$files($$PWD/Linux/share/sasm/include/*.inc) $$PWD/macos/include/macos.inc
 macincludes.path = Contents/Resources/sasm/include
 QMAKE_BUNDLE_DATA += data macdata macincludes macros examples
 INSTALLS -= binfile data shortcutfiles docfiles
}

CODECFORTR      = UTF-8
TRANSLATIONS    += language_ru.ts \
                   language_tr.ts \
                   language_zh.ts \
                   language_de.ts \
                   language_it.ts \
                   language_pl.ts \
                   language_he.ts \
                   language_es.ts \
                   language_pt_PT.ts \
                   language_fr.ts \
                   language_pt_BR.ts \
                   language_ja.ts
