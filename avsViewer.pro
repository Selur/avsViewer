QT += core \
    gui \
    network
CONFIG += qt
CONFIG += precompile_header
CONFIG += console

UI_DIR = uiHeaders
TEMPLATE = app

# Qt 5+ adjustments
greaterThan(QT_MAJOR_VERSION, 4) { # QT5+
  QT += widgets # for all widgets
  lessThan(QT_MAJOR_VERSION, 6) {
    QT += multimedia # for QSound
  }
  greaterThan(QT_MAJOR_VERSION, 5):greaterThan(QT_MINOR_VERSION, 1) { # QT6.2+
    QT += multimedia
  }
}

win32-msvc* {
    message(Building for Windows using Qt $$QT_VERSION)
    lessThan(QT_MAJOR_VERSION, 6) {
      CONFIG += c++11 # C++11 support
    } else {
      CONFIG += c++17 # C++11 support
      QMAKE_CXXFLAGS += /std:c++17
    }
    QMAKE_LFLAGS += /STACK:64000000
    QMAKE_CXXFLAGS += -bigobj

    greaterThan(QT_MAJOR_VERSION, 5) {
      QMAKE_LFLAGS += /entry:mainCRTStartup
    }
    QMAKE_CFLAGS_RELEASE += -WX
    QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO += -WX
    QMAKE_CFLAGS_RELEASE += -link notelemetry.obj

    # some Windows headers violate strictStrings rules
    QMAKE_CXXFLAGS_RELEASE -= -Zc:strictStrings
    QMAKE_CFLAGS_RELEASE -= -Zc:strictStrings
    QMAKE_CFLAGS -= -Zc:strictStrings
    QMAKE_CXXFLAGS -= -Zc:strictStrings
    QMAKE_CXXFLAGS_RELEASE += /Zc:__cplusplus
    QMAKE_CFLAGS_RELEASE += /Zc:__cplusplus
    QMAKE_CFLAGS += /Zc:__cplusplus
    QMAKE_CXXFLAGS += /Zc:__cplusplus

    greaterThan(QT_MAJOR_VERSION, 4):greaterThan(QT_MINOR_VERSION, 4) { # Qt5.5
      lessThan(QT_MAJOR_VERSION, 6) {
        QT += winextras
      }
      DEFINES += NOMINMAX
    }
    QMAKE_CXXFLAGS += -permissive-
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
