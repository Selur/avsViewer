QT += core \
    gui \
    network
CONFIG += qt
CONFIG += console

win32-msvc* {
    message(Building for Windows using Qt $$QT_VERSION)
    lessThan(QT_MAJOR_VERSION, 6) {
      CONFIG += c++11 # C++11 support
    } else {
      CONFIG += c++17 # C++11 support
      QMAKE_CXXFLAGS += /std:c++17
      QMAKE_CXXFLAGS += /Zc:__cplusplus
      QMAKE_CXXFLAGS_RELEASE += /Zc:__cplusplus
    }
    QMAKE_CXXFLAGS_RELEASE += -MP
    QMAKE_LFLAGS += /LARGEADDRESSAWARE
    # for Windows XP compatibility
    contains(QMAKE_HOST.arch, x86_64):QMAKE_LFLAGS += /SUBSYSTEM:WINDOWS,5.02 # Windows XP 64bit
    else:QMAKE_LFLAGS += /SUBSYSTEM:WINDOWS,5.01 # Windows XP 32bit
}
greaterThan(QT_MAJOR_VERSION, 4) { # QT5+
    QT += widgets # for all widgets
    win32-msvc*:DEFINES += NOMINMAX
}
CODECFORSRC = UTF-8
CODECFORTR = UTF-8
TEMPLATE = app
contains(QMAKE_HOST.arch, x86_64) {
  TARGET = avsViewer64
} else {
  TARGET = avsViewer
}

HEADERS += LocalSocketIpcServer.h \
    LocalSocketIpcClient.h \
    avsViewer.h \
    MarkSlider.h \
    MoveScrollArea.h
SOURCES += LocalSocketIpcServer.cpp \
    LocalSocketIpcClient.cpp \
    avsViewer.cpp \
    MarkSlider.cpp \
    main.cpp \
    MoveScrollArea.cpp
FORMS += viewer.ui
RESOURCES +=


INCLUDEPATH += "C:\Program Files (x86)\AviSynth+\FilterSDK\include"
