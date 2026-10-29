#!/usr/bin/env bash
# Shared Qt 6 discovery. Explicit paths let Homebrew Qt 5 remain linked.
QT_PREFIX="${QT_PREFIX:-$(brew --prefix qtbase)}"
QMAKE="${QMAKE:-$QT_PREFIX/bin/qmake}"
QT_BIN="${QT_BIN:-$QT_PREFIX/bin}"
mkdir -p "$BUILD_DIR"
cat > "$BUILD_DIR/qt.conf" <<CONFIG
[Paths]
Prefix=$QT_PREFIX
ArchData=share/qt
Data=share/qt
Libraries=lib
Headers=include
Binaries=bin
LibraryExecutables=share/qt/libexec
HostLibraryExecutables=share/qt/libexec
Plugins=share/qt/plugins
HostData=share/qt
CONFIG
QMAKE_ARGS=(-qtconf "$BUILD_DIR/qt.conf")
