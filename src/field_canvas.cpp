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

#include "disp_holder.h"
#include "field_painter.h"

#include "ball_painter.h"
#include "player_painter.h"
#include "score_board_painter.h"
#include "team_graphic_painter.h"
#include "draw_info_painter.h"

#include "options.h"

#include <iostream>
#include <cmath>

/*-------------------------------------------------------------------*/
/*!

*/
FieldCanvas::FieldCanvas( DispHolder & disp_holder )
    :
#ifdef USE_GLWIDGET
    QGLWidget( QGLFormat( QGL::SampleBuffers ) ),
#else
    QWidget( /* parent, flags */ ),
#endif
    M_disp_holder( disp_holder ),
    M_monitor_menu( static_cast< QMenu * >( 0 ) )
{
    M_focus_move_mouse = &M_mouse_state[0];
    M_measure_mouse = &M_mouse_state[1];
    M_menu_mouse = &M_mouse_state[2];

    this->setMouseTracking( true ); // need for the MouseMoveEvent
    this->setFocusPolicy( Qt::WheelFocus );

    createPainters();
}

/*-------------------------------------------------------------------*/
/*!

*/
FieldCanvas::~FieldCanvas()
{

}

/*-------------------------------------------------------------------*/
/*!

*/
QMenu *
FieldCanvas::createPopupMenu()
{
    // create & set context menus
    M_monitor_menu = new QMenu( this );
    M_monitor_menu->addAction( tr( "Drop Ball" ),
                               this, SLOT( dropBall() ) );
    M_monitor_menu->addAction( tr( "Free Kick Left" ),
                               this, SLOT( freeKickLeft() ) );
    M_monitor_menu->addAction( tr( "Free Kick Right" ),
                               this, SLOT( freeKickRight() ) );

    return M_monitor_menu;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FieldCanvas::createPainters()
{
    M_field_painter = boost::shared_ptr< FieldPainter >( new FieldPainter( M_disp_holder ) );

    M_painters.push_back( boost::shared_ptr< PainterInterface >( new ScoreBoardPainter( M_disp_holder ) ) );
    M_painters.push_back( boost::shared_ptr< PainterInterface >( new TeamGraphicPainter( M_disp_holder ) ) );
    M_painters.push_back( boost::shared_ptr< PainterInterface >( new PlayerPainter( M_disp_holder ) ) );
    M_painters.push_back( boost::shared_ptr< PainterInterface >( new BallPainter( M_disp_holder ) ) );
    M_painters.push_back( boost::shared_ptr< PainterInterface >( new DrawInfoPainter( M_disp_holder ) ) );
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

        // selectPlayer( event->pos() );
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

    if ( M_focus_move_mouse->isDragged() )
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
        new_x -= ( event->pos().x() - M_focus_move_mouse->draggedPoint().x() );
        new_y -= ( event->pos().y() - M_focus_move_mouse->draggedPoint().y() );
        emit focusChanged( QPoint( new_x, new_y ) );
    }

    for ( int i = 0; i < 3; ++i )
    {
        M_mouse_state[i].moved( event->pos() );
    }

    if ( M_measure_mouse->isDragged() )
    {
        static QRect s_last_rect;

        if ( this->cursor().shape() != Qt::CrossCursor )
        {
            this->setCursor( QCursor( Qt::CrossCursor ) );
        }

        QRect new_rect = QRect( M_measure_mouse->pressedPoint(),
                                M_measure_mouse->draggedPoint() ).normalized();
        new_rect.adjust( -32, -32, 32, 32 );
        if ( new_rect.right() < M_measure_mouse->draggedPoint().x() + 256 )
        {
            new_rect.setRight( M_measure_mouse->draggedPoint().x() + 256 );
        }
        // draw mouse measure
        this->update( s_last_rect.unite( new_rect ) );
        s_last_rect = new_rect;
    }

    emit mouseMoved( event->pos() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FieldCanvas::contextMenuEvent( QContextMenuEvent * event )
{
    if ( event->reason() == QContextMenuEvent::Mouse )
    {
        M_mouse_state[2].released();
        if ( M_monitor_menu )
        {
            M_monitor_menu->exec( event->globalPos() );
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FieldCanvas::paintEvent( QPaintEvent * )
{
    QPainter painter( this );

    draw( painter );

    if ( M_measure_mouse->isDragged() )
    {
        drawMouseMeasure( painter );
    }

    drawRecoveringState( painter );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FieldCanvas::updateFocus()
{
    DispConstPtr disp = M_disp_holder.currentDisp();

    if ( ! disp )
    {
        return;
    }

    // if auto select mode, update the ball nearest player
    if ( Options::instance().playerAutoSelect() )
    {
        const rcss::rcg::ShowInfoT & show = disp->show_;

        Options::PlayerSelectType old_type = Options::instance().playerSelectType();

        float min_dist2 = 40000.0f;

        rcss::rcg::Side side = rcss::rcg::NEUTRAL;
        int unum = 0;

        const int first = ( old_type == Options::SELECT_AUTO_RIGHT
                            ? 11
                            : 0 );
        const int last = ( old_type == Options::SELECT_AUTO_LEFT
                           ? 11
                           : 22 );
        for ( int i = first; i < last; ++i )
        {
            if ( show.player_[i].state_ != 0 )
            {
                float d2
                    = std::pow( show.ball_.x_ - show.player_[i].x_, 2 )
                    + std::pow( show.ball_.y_ - show.player_[i].y_, 2 );
                if ( d2 < min_dist2 )
                {
                    min_dist2 = d2;
                    side = show.player_[i].side();
                    unum = show.player_[i].unum_;
                }
            }
        }

        if ( unum != 0 )
        {
            Options::instance().setSelectedNumber( side, unum );
        }
    }

    // update focus point
    if ( Options::instance().focusType() == Options::FOCUS_BALL )
    {
        Options::instance().setFocusPointReal( disp->show_.ball_.x_,
                                               disp->show_.ball_.y_ );
    }
    else if ( Options::instance().focusType() == Options::FOCUS_PLAYER
              && Options::instance().selectedNumber() != 0 )
    {
        int id = Options::instance().selectedNumber();
        if ( id < 0 )
        {
            id = -1*id + 11;
        }
        id -= 1;

        if ( disp->show_.player_[id].state_ != 0 )
        {
            Options::instance().setFocusPointReal( disp->show_.player_[id].x_,
                                                   disp->show_.player_[id].y_ );
        }
    }
    else
    {
        // already set
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FieldCanvas::draw( QPainter & painter )
{
    updateFocus();
    Options::instance().updateFieldSize( this->width(), this->height() );

    M_field_painter->draw( painter );

    if ( ! M_disp_holder.currentDisp() )
    {
        return;
    }

    for ( std::vector< boost::shared_ptr< PainterInterface > >::iterator it = M_painters.begin();
          it != M_painters.end();
          ++it )
    {
        (*it)->draw( painter );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FieldCanvas::drawMouseMeasure( QPainter & painter )
{
    const Options & opt = Options::instance();

    QPoint start_point = M_measure_mouse->pressedPoint();
    QPoint end_point = M_measure_mouse->draggedPoint();

    if ( opt.antiAliasing() )
    {
        painter.setRenderHint( QPainter::Antialiasing, false );
    }

    // draw straight line
    painter.setPen( opt.measureLinePen() );
    painter.setBrush( Qt::NoBrush );
    painter.drawLine( start_point, end_point );

    // draw mark
    painter.setPen( opt.measureMarkPen() );
    painter.drawEllipse( start_point.x() - 2,
                         start_point.y() - 2,
                         4,
                         4 );
    painter.drawEllipse( end_point.x() - 2,
                         end_point.y() - 2,
                         4,
                         4 );

    if ( opt.antiAliasing() )
    {
        painter.setRenderHint( QPainter::Antialiasing );
    }

    // draw distance & angle text
    painter.setFont( opt.measureFont() );
    painter.setPen( opt.measureFontPen() );

    char buf[64];

    // draw start point value
    QPointF start_real( opt.fieldX( start_point.x() ),
                        opt.fieldY( start_point.y() ) );
    snprintf( buf, 64,
              "(%.2f,%.2f)",
              start_real.x(),
              start_real.y() );
    painter.drawText( start_point,
                      QString::fromAscii( buf ) );

    if ( std::abs( start_point.x() - end_point.x() ) < 1
         && std::abs( start_point.y() - end_point.y() ) < 1 )
    {
        return;
    }

    // draw end point value
    QPointF end_real( opt.fieldX( end_point.x() ),
                      opt.fieldY( end_point.y() ) );
    snprintf( buf, 64,
              "(%.2f,%.2f)",
              end_real.x(),
              end_real.y() );
    painter.drawText( end_point.x(),
                      end_point.y(),
                      QString::fromAscii( buf ) );

    // draw relative coordinate value
    painter.setPen( opt.measureFontPen2() );

    QPointF rel( end_real - start_real );
    double r = std::sqrt( std::pow( rel.x(), 2.0 ) + std::pow( rel.y(), 2.0 ) );
    double th = ( rel.x() == 0.0 && rel.y() == 0.0
                   ? 0.0
                   : std::atan2( rel.y(), rel.x() ) * 180.0 / M_PI );

    snprintf( buf, 64,
              "rel(%.2f,%.2f) r%.2f th%.1f",
              rel.x(), rel.y(), r, th );

    int dist_add_y = ( end_point.y() > start_point.y()
                       ? + painter.fontMetrics().height()
                       : - painter.fontMetrics().height() );
    painter.drawText( end_point.x(),
                      end_point.y() + dist_add_y,
                      QString::fromAscii( buf ) );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FieldCanvas::drawRecoveringState( QPainter & painter )
{
    const Options & opt = Options::instance();

    if ( ! opt.bufferingMode()
         || ! opt.bufferRecoverMode()
         || M_disp_holder.currentIndex() == DispHolder::INVALID_INDEX )
    {
        return;
    }

    static const int divs = 16;
    static const double angle_step = 360.0 / divs;

    static int s_current_index = 0;

    painter.save();

    painter.translate( QPoint( this->width() / 2,
                               this->height() / 2 ) );
#if 0
    size_t cur = M_disp_holder.currentIndex() == DispHolder::INVALID_INDEX
        ? 0
        : M_disp_holder.currentIndex();
    int caching = M_disp_holder.dispCont().size() - cur - 1;

    painter.setPen( Qt::white );
    painter.setBrush( Qt::gray );
    QFont font = opt.scoreBoardFont();
    font.setPointSize( 18 );
    font.setBold( true );
    painter.setFont( font );
    painter.drawText( QRect( -10, -10, 20, 20 ),
                      Qt::AlignCenter | Qt::TextSingleLine,
                      QString::number( caching ) );
#endif
    painter.rotate( angle_step * s_current_index );

    painter.setPen( Qt::NoPen );

    const int x_size = std::max( 2, Options::WAITING_ANIMATION_SIZE/4 );
    const int y_size = std::max( 2, x_size/4 );
    const int x = std::max( 2, Options::WAITING_ANIMATION_SIZE/2 - x_size );

    for ( int i = 0; i < divs; ++i )
    {
        int c = std::max( 95, 255 - ( i * 32 ) );
        //int c = std::min( 191, i * 32 );
        painter.setBrush( QColor( c, c, c ) );
        painter.drawRoundedRect( x, 0,
                                 x_size, y_size,
                                 4.0, 4.0 );
        painter.rotate( -angle_step );
    }

    painter.restore();

    s_current_index = ( s_current_index + 1 ) % divs;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FieldCanvas::dropBall()
{
    emit dropBall( M_menu_mouse->pressedPoint() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FieldCanvas::freeKickLeft()
{
    emit freeKickLeft( M_menu_mouse->pressedPoint() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FieldCanvas::freeKickRight()
{
    emit freeKickRight( M_menu_mouse->pressedPoint() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FieldCanvas::changePlayMode( int mode )
{
    emit playModeChanged( mode, M_menu_mouse->pressedPoint() );
}
