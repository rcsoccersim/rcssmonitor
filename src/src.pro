TEMPLATE = app
QT += network opengl
TARGET = rcssmonitor
DESTDIR = ../bin

DEPENDPATH += .

INCLUDEPATH += . ..
win32 {
  INCLUDEPATH +=  ../../boost
}
unix {
  INCLUDEPATH += /opt/local/include
}
macx {
  INCLUDEPATH += /opt/local/include
}

win32 {
  LIBS += ../../zlib123-dll/zlib1.dll -lwsock32
}
unix {
  LIBS += -L/opt/local/lib
  LIBS += -lboost_program_options-mt -lz
}
macx {
  LIBS += -L/opt/local/lib
  LIBS += -lboost_program_options-mt -lz
}

DEFINES += USE_GL_WIDGET

DEFINES += HAVE_LIBZ
win32 {
  DEFINES += HAVE_WINDOWS_H
}
unix {
  DEFINES += HAVE_NETINET_IN_H
  DEFINES += HAVE_BOOST_PROGRAM_OPTIONS
}
macx {
  DEFINES += HAVE_NETINET_IN_H
  DEFINES += HAVE_BOOST_PROGRAM_OPTIONS
}
DEFINES += PACKAGE="\\\"rcssmonitor\\\""
DEFINES += PACKAGE_NAME="\\\"rcssmonitor\\\""
DEFINES += VERSION="\\\"x.x.x\\\""

CONFIG += qt warn_on release
win32 {
  CONFIG += windows
}

#QMAKE_LFLAGS_DEBUG += -static
#QMAKE_LFLAGS_DEBUG += -pg
#QMAKE_CFLAGS_DEBUG += -pg -static -DQABSTRACTSOCKET_DEBUG
#QMAKE_CXXFLAGS += -static
#QMAKE_CXXFLAGS_DEBUG += -pg -static -DQABSTRACTSOCKET_DEBUG

# Input
HEADERS += \
	main_window.h \
	options.h

SOURCES += \
	main_window.cpp \
	options.cpp \
	main.cpp

nodist_rcsslogplayer_SOURCES = \
	moc_main_window.cpp

win32 {
RC_FILE = rcssmonitor.rc
}
macx {
ICON = ../icons/rcss.icns
}
