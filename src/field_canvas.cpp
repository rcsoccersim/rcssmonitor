// -*-c++-*-

/*!
  \file field_canvas.cpp
  \brief main field canvas class Source File.
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
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <QtGui>

#include "field_canvas.h"

#include "field_painter.h"
#include "options.h"

#include <iostream>

/*-------------------------------------------------------------------*/
/*!

*/
FieldCanvas::FieldCanvas()
    :
#ifdef USE_GLWIDGET
    QGLWidget( QGLFormat( QGL::SampleBuffers ) ),
#else
    QWidget( /* parent, flags */ ),
#endif
    M_monitor_menu( static_cast< QMenu * >( 0 ) )
{
    this->setMouseTracking( true ); // need for the MouseMoveEvent
    this->setFocusPolicy( Qt::WheelFocus );

    readSettings();

    createPainters();
}

/*-------------------------------------------------------------------*/
/*!

*/
FieldCanvas::~FieldCanvas()
{
    writeSettings();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FieldCanvas::setMonitorMenu( QMenu * menu )
{
    if ( M_monitor_menu )
    {
        delete M_monitor_menu;
        M_monitor_menu = static_cast< QMenu * >( 0 );
    }

    M_monitor_menu = menu;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FieldCanvas::readSettings()
{
    QSettings settings( Options::CONF_FILE,
                        QSettings::IniFormat );

    settings.beginGroup( "FieldCanvas" );

//     QVariant val;

//     val = settings.value( "measure_line_pen" );
//     if ( val.isValid() ) M_measure_line_pen.setColor( val.toString() );

//     val = settings.value( "measure_mark_pen" );
//     if ( val.isValid() ) M_measure_mark_pen.setColor( val.toString() );

//     val = settings.value( "measure_font_pen" );
//     if ( val.isValid() ) M_measure_font_pen.setColor( val.toString() );

//     val = settings.value( "measure_font_pen2" );
//     if ( val.isValid() ) M_measure_font_pen2.setColor( val.toString() );

//     val = settings.value( "measure_font" );
//     if ( val.isValid() ) M_measure_font.fromString( val.toString() );

    settings.endGroup();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FieldCanvas::writeSettings()
{
    QSettings settings( Options::CONF_FILE,
                        QSettings::IniFormat );

    settings.beginGroup( "FieldCanvas" );

//     settings.setValue( "measure_line_pen", M_measure_line_pen.color().name() );
//     settings.setValue( "measure_mark_pen", M_measure_mark_pen.color().name() );
//     settings.setValue( "measure_font_pen", M_measure_font_pen.color().name() );
//     settings.setValue( "measure_font_pen2", M_measure_font_pen2.color().name() );
//     settings.setValue( "measure_font", M_measure_font.toString() );

    settings.endGroup();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FieldCanvas::createPainters()
{
    M_field_painter = boost::shared_ptr< FieldPainter >( new FieldPainter() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FieldCanvas::mouseDoubleClickEvent( QMouseEvent * event )
{
    this->unsetCursor();

    if ( event->button() == Qt::LeftButton )
    {
        emit focusChanged( event->pos() );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FieldCanvas::mousePressEvent( QMouseEvent * event )
{
    if ( event->button() == Qt::LeftButton )
    {
        M_mouse_state[0].pressed( event->pos() );
        if ( M_mouse_state[1].isMenuFailed() )
        {
            M_mouse_state[1].setMenuFailed( false );
        }

        if ( event->modifiers() == Qt::ControlModifier )
        {
            emit focusChanged( event->pos() );
        }
//         else
//         {
//             selectPlayer( event->pos() );
//         }
    }
    else if ( event->button() == Qt::MidButton )
    {
        M_mouse_state[1].pressed( event->pos() );
        if ( M_mouse_state[1].isMenuFailed() )
        {
            M_mouse_state[1].setMenuFailed( false );
        }
    }
    else if ( event->button() == Qt::RightButton )
    {
        M_mouse_state[2].pressed( event->pos() );
        if ( M_mouse_state[2].isMenuFailed() )
        {
            M_mouse_state[2].setMenuFailed( false );
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FieldCanvas::mouseReleaseEvent( QMouseEvent * event )
{
    this->unsetCursor();

    if ( event->button() == Qt::LeftButton )
    {
        M_mouse_state[0].released();
    }
    else if ( event->button() == Qt::MidButton )
    {
        M_mouse_state[1].released();
    }
    else if ( event->button() == Qt::RightButton )
    {
        M_mouse_state[2].released();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FieldCanvas::mouseMoveEvent( QMouseEvent * event )
{
    if ( this->cursor().shape() == Qt::BlankCursor )
    {
        this->unsetCursor();
    }

    if ( M_mouse_state[0].isDragged() )
    {
#if QT_VERSION >= 0x040200
        if ( this->cursor().shape() != Qt::ClosedHandCursor )
        {
            this->setCursor( QCursor( Qt::ClosedHandCursor ) );
        }
#else
        if ( this->cursor().shape() != Qt::SizeAllCursor )
        {
            this->setCursor( QCursor( Qt::SizeAllCursor ) );
        }
#endif

        int new_x = Options::instance().screenX( Options::instance().focusPoint().x() );
        int new_y = Options::instance().screenY( Options::instance().focusPoint().y() );
        new_x -= ( event->pos().x() - M_mouse_state[0].draggedPoint().x() );
        new_y -= ( event->pos().y() - M_mouse_state[0].draggedPoint().y() );
        emit focusChanged( QPoint( new_x, new_y ) );
    }

    for ( int i = 0; i < 3; ++i )
    {
        M_mouse_state[i].moved( event->pos() );
    }

    if ( M_mouse_state[2].isDragged() )
    {
        static QRect s_last_rect;

        if ( this->cursor().shape() != Qt::CrossCursor )
        {
            this->setCursor( QCursor( Qt::CrossCursor ) );
        }

        QRect new_rect
            = QRect( M_mouse_state[2].pressedPoint(),
                     M_mouse_state[2].draggedPoint() ).normalized();
        new_rect.adjust( -32, -32, 32, 32 );
        if ( new_rect.right() < M_mouse_state[2].draggedPoint().x() + 256 )
        {
            new_rect.setRight( M_mouse_state[2].draggedPoint().x() + 256 );
        }
        // draw mouse measure
        this->update( s_last_rect.unite( new_rect ) );
        s_last_rect = new_rect;
    }

    emit mouseMoved( event->pos() );
}

void
FieldCanvas::contextMenuEvent( QContextMenuEvent * event )
{
    if ( event->reason() == QContextMenuEvent::Mouse )
    {
        M_monitor_menu->exec( event->globalPos() );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FieldCanvas::paintEvent( QPaintEvent * )
{
    QPainter painter( this );

    if ( Options::instance().antiAliasing() )
    {
#ifdef USE_GLWIDGET
        painter.setRenderHint( QPainter::HighQualityAntialiasing );
#else
        painter.setRenderHint( QPainter::Antialiasing );
#endif
    }

    draw( painter );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FieldCanvas::draw( QPainter & painter )
{
    Options::instance().updateFieldSize( this->width(), this->height() );

    M_field_painter->draw( painter );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FieldCanvas::dropBall()
{
    emit dropBall( M_mouse_state[0].pressedPoint() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FieldCanvas::freeKickLeft()
{
    emit freeKickLeft( M_mouse_state[0].pressedPoint() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FieldCanvas::freeKickRight()
{
    emit freeKickRight( M_mouse_state[0].pressedPoint() );
}
