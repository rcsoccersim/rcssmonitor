TEMPLATE = app
# QT += network opengl
QT += network
TARGET = rcssmonitor
DESTDIR = ../bin

DEPENDPATH += .

INCLUDEPATH += . ..
win32 {
  INCLUDEPATH +=  c:/Qt/boost-include
}
unix {
  INCLUDEPATH += /opt/local/include
}
macx-g++ {
  INCLUDEPATH += /opt/local/include
}

win32 {
#  LIBS += ../../zlib123-dll/zlib1.dll -lwsock32
#  LIBS += c:/MinGW/boost-lib/libboost_program_options-mgw34-mt.lib
  LIBS += c:/Qt/boost-lib/boost_program_options-mgw44-mt-1_42.dll
  LIBS += -lwsock32
}
unix {
  LIBS += -L/opt/local/lib
  LIBS += -lboost_program_options-mt -lz
}
macx-g++ {
  LIBS += -L/opt/local/lib
  LIBS += -lboost_program_options-mt -lz
}

# DEFINES += USE_GL_WIDGET

DEFINES += HAVE_LIBZ
win32 {
  DEFINES += HAVE_WINDOWS_H
  DEFINES += HAVE_BOOST_PROGRAM_OPTIONS
}
unix {
  DEFINES += HAVE_NETINET_IN_H
  DEFINES += HAVE_BOOST_PROGRAM_OPTIONS
}
macx-g++ {
  DEFINES += HAVE_NETINET_IN_H
  DEFINES += HAVE_BOOST_PROGRAM_OPTIONS
}
DEFINES += PACKAGE="\\\"rcssmonitor\\\""
DEFINES += PACKAGE_NAME="\\\"rcssmonitor\\\""
DEFINES += VERSION="\\\"14.1.1\\\""

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
	rcsslogplayer/handler.h \
	rcsslogplayer/parser.h \
	rcsslogplayer/types.h \
	rcsslogplayer/util.h \
	angle_deg.h \
	ball_painter.h \
	circle_2d.h \
	config_dialog.h \
	disp_holder.h \
	draw_info_painter.h \
	field_canvas.h \
	field_painter.h \
	line_2d.h \
	log_player.h \
	main_window.h \
	monitor_client.h \
	mouse_state.h \
	options.h \
	painter_interface.h \
	player_painter.h \
	player_type_dialog.h \
	score_board_painter.h \
	team_graphic.h \
	team_graphic_painter.h \
	vector_2d.h

SOURCES += \
	rcsslogplayer/parser.cpp \
	rcsslogplayer/types.cpp \
	rcsslogplayer/util.cpp \
	angle_deg.cpp \
	ball_painter.cpp \
	circle_2d.cpp \
	config_dialog.cpp \
	disp_holder.cpp \
	draw_info_painter.cpp \
	field_canvas.cpp \
	field_painter.cpp \
	line_2d.cpp \
	log_player.cpp \
	main_window.cpp \
	monitor_client.cpp \
	options.cpp \
	player_painter.cpp \
	player_type_dialog.cpp \
	score_board_painter.cpp \
	team_graphic.cpp \
	team_graphic_painter.cpp \
	vector_2d.cpp \
	main.cpp

nodist_rcsslogplayer_SOURCES = \
	moc_main_window.cpp

win32 {
RC_FILE = rcssmonitor.rc
}
macx-g++ {
ICON = ../icons/rcss.icns
}
