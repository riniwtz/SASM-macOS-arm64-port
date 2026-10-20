include(SASM.pro)
QT += testlib
TARGET = sasm-tests
SOURCES -= main.cpp
SOURCES += macos/tests.cpp
CONFIG += testcase
