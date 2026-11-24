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

#include <QWidget>

#include <QImage>
#include <QPen>
#include <QFont>

#include "mouse_state.h"
#include "referee_target.h"

#include <cstddef>
#include <memory>

class DispHolder;
class FieldPainter;
class PainterInterface;

class FieldCanvas
    : public QWidget {

    Q_OBJECT

private:

    DispHolder & M_disp_holder;

    std::shared_ptr< FieldPainter > M_field_painter;
    std::vector< std::shared_ptr< PainterInterface > > M_painters;

    //! 0: left, 1: middle, 2: right
    MouseState M_mouse_state[3];

    const MouseState * M_focus_move_mouse;
    const MouseState * M_measure_mouse;
    const MouseState * M_menu_mouse;

    // not used
    FieldCanvas() = delete;
    FieldCanvas( const FieldCanvas & ) = delete;
    const FieldCanvas & operator=( const FieldCanvas & ) = delete;

public:

    explicit
    FieldCanvas( DispHolder & disp_holder );
    ~FieldCanvas();

    /*!
      \brief draw a frame into an image, as the view draws it
      (docs/plan/2026-11-22-03_frame-export.md 4.1): the state of the view
      and the frame shown are set back afterward (FrameScope). The line of
      the measure by the middle button is not drawn. When zoomed, the scale
      follows the ratio of the heights, to show the same range as the view.
      \param index the frame (an index out of the frames: the frame shown)
      \param pixels the size of the image
      \param dpr the device pixel ratio of the image: the frame is drawn in
      the logical size pixels / dpr, and the texts are dpr times as large
      as in that size
      \return the image (Format_RGB32), null if the size is empty
    */
    QImage renderFrame( const std::size_t index,
                        const QSize & pixels,
                        const qreal dpr );

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

    /*!
      \brief draw the frame shown
      \param logical the size of the canvas (the window of the painter)
    */
    void draw( QPainter & painter,
               const QSize & logical );
    void drawMouseMeasure( QPainter & painter );

signals:

    /*!
      \brief a right click asks for the referee's menu (RefereeMenu)
      \param target the point on the field where the right button was pressed
      \param global_pos where to show the menu on the screen
    */
    void refereeMenuRequested( const RefereeTarget & target,
                               const QPoint & global_pos );

//     void playerMoved( const QPoint & point );
//     void playerSelected( int number );

    void focusChanged( const QPoint & point );
    void mouseMoved( const QPoint & point );

};

#endif
