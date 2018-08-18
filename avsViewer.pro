QT += core \
    gui \
    network
CONFIG += qt
CONFIG += console

win32-msvc* { 
    message(Building for Windows using Qt $$QT_VERSION)
    CONFIG += c++11
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
TARGET = avsViewer
HEADERS += LocalSocketIpcServer.h \
    LocalSocketIpcClient.h \
    avsViewer.h \
    mywindows.h \
    MarkSlider.h \
    stdafx.h \
    avisynth.h \
    MoveScrollArea.h
SOURCES += LocalSocketIpcServer.cpp \
    LocalSocketIpcClient.cpp \
    avsViewer.cpp \
    MarkSlider.cpp \
    interface.cpp \
    main.cpp \
    MoveScrollArea.cpp
FORMS += viewer.ui
RESOURCES += 
