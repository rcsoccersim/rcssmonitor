// -*-c++-*-

/*!
	\file field_canvas.h
	\brief field canvas class Header File.
*/

/*
 *Copyright:

 Copyright (C) The RoboCup Soccer Server Maintenance Group.
 Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.	If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifndef RCSSMONITOR_FIELD_CANVAS_H
#define RCSSMONITOR_FIELD_CANVAS_H

#ifdef USE_GLWIDGET
#include <QGLWidget>
#else
#include <QWidget>
#endif

#include <QPen>
#include <QFont>

#include "mouse_state.h"

#include <boost/shared_ptr.hpp>

class QMenu;

class DispHolder;
class FieldPainter;
class PainterInterface;

class FieldCanvas
		:
#ifdef USE_GLWIDGET
    public QGLWidget
#else
    public QWidget
#endif
{

    Q_OBJECT

private:

    DispHolder & M_disp_holder;

    QMenu * M_monitor_menu;

    boost::shared_ptr< FieldPainter > M_field_painter;
    std::vector< boost::shared_ptr< PainterInterface > > M_painters;

    //! 0: left, 1: middle, 2: right
    MouseState M_mouse_state[3];

    const MouseState * M_focus_move_mouse;
    const MouseState * M_measure_mouse;
    const MouseState * M_menu_mouse;

    // not used
    FieldCanvas();
    FieldCanvas( const FieldCanvas & );
    const FieldCanvas & operator=( const FieldCanvas & );

public:

    explicit
    FieldCanvas( DispHolder & disp_holder );
    ~FieldCanvas();

    QMenu * createPopupMenu();

private:

    void createPainters();

    void updateFocus();

protected:

    // overrided methods
    void mouseDoubleClickEvent( QMouseEvent * event );
    void mousePressEvent( QMouseEvent * event );
    void mouseReleaseEvent( QMouseEvent * event );
    void mouseMoveEvent( QMouseEvent * event );

    void contextMenuEvent( QContextMenuEvent * event );

    void paintEvent( QPaintEvent * );

private:

    void draw( QPainter & painter );
    void drawMouseMeasure( QPainter & painter );
    void drawRecoveringState( QPainter & painter );

public slots:

    void dropBall();
    void freeKickLeft();
    void freeKickRight();
    void changePlayMode( int mode );

signals:

    void dropBall( const QPoint & pos );
    void freeKickLeft( const QPoint & pos );
    void freeKickRight( const QPoint & pos );
    void playModeChanged( int mode,
                          const QPoint & pos );

//     void playerMoved( const QPoint & point );
//     void playerSelected( int number );

    void focusChanged( const QPoint & point );
    void mouseMoved( const QPoint & point );

};

#endif
