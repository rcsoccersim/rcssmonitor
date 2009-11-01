// -*-c++-*-

/*!
  \file ball_painter.cpp
  \brief ball painter class Source File.
*/

/*
 *Copyright:

 Copyright (C)  The RoboCup Soccer Server Maintenance Group.
 Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
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

#include "ball_painter.h"

#include "main_data.h"
#include "options.h"
#include "vector_2d.h"

#include <rcsslogplayer/types.h>

#include <vector>

/*-------------------------------------------------------------------*/
/*!

*/
BallPainter::BallPainter( const MainData & main_data )
    : M_main_data( main_data )
    , M_ball_pen( QColor( 255, 255, 255 ), 0, Qt::SolidLine )
    , M_ball_vel_pen( QColor( 255, 0, 0 ), 0, Qt::SolidLine )
    , M_ball_brush( QColor( 255, 255, 255 ), Qt::SolidPattern )
{
    readSettings();
}

/*-------------------------------------------------------------------*/
/*!

*/
BallPainter::~BallPainter()
{
    writeSettings();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
BallPainter::readSettings()
{
#ifndef Q_WS_WIN
    QSettings settings( QDir::homePath() + "/.rcsslogplayer",
                        QSettings::IniFormat );
#else
    QSettings settings( QDir::currentPath() + "/rcsslogplayer.ini",
                        QSettings::IniFormat );
#endif

    settings.beginGroup( "BallPainter" );

    QVariant val;

    val = settings.value( "ball_pen" );
    if ( val.isValid() ) M_ball_pen.setColor( val.toString() );

    val = settings.value( "ball_vel_pen" );
    if ( val.isValid() ) M_ball_vel_pen.setColor( val.toString() );

    val = settings.value( "ball_brush" );
    if ( val.isValid() ) M_ball_brush.setColor( val.toString() );

    settings.endGroup();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
BallPainter::writeSettings()
{
#ifndef Q_WS_WIN
    QSettings settings( QDir::homePath() + "/.rcsslogplayer",
                        QSettings::IniFormat );
#else
    QSettings settings( QDir::currentPath() + "/rcsslogplayer.ini",
                        QSettings::IniFormat );
#endif

    settings.beginGroup( "BallPainter" );

    settings.setValue( "ball_pen", M_ball_pen.color().name() );
    settings.setValue( "ball_vel_pen", M_ball_vel_pen.color().name() );
    settings.setValue( "ball_brush", M_ball_brush.color().name() );

    settings.endGroup();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
BallPainter::draw( QPainter & painter )
{
    const Options & opt = Options::instance();

    if ( ! opt.showBall()
         || opt.minimumMode() )
    {
        return;
    }

    DispConstPtr disp = M_main_data.getDispInfo( M_main_data.index() );

    if ( ! disp )
    {
        return;
    }

    const rcss::rcg::ServerParamT & sparam = M_main_data.serverParam();

    // decide radius
    const int ball_radius = ( opt.ballSize() >= 0.01
                              ? opt.scale( opt.ballSize() )
                              : std::max( 1, opt.scale( sparam.ball_size_ ) ) );
    const int kickable_radius
        = std::max( 1, opt.scale( sparam.player_size_
                                  + sparam.kickable_margin_
                                  + sparam.ball_size_ ) );
    const int ix = opt.screenX( disp->show_.ball_.x_ );
    const int iy = opt.screenY( disp->show_.ball_.y_ );

    // draw ball body
    painter.setPen( Qt::NoPen );
    painter.setBrush( M_ball_brush );
    painter.drawEllipse( ix - ball_radius,
                         iy - ball_radius,
                         ball_radius * 2,
                         ball_radius * 2 );

    // draw kickable margin
    painter.setPen( M_ball_pen );
    painter.setBrush( Qt::NoBrush );
    painter.drawEllipse( ix - kickable_radius,
                         iy - kickable_radius,
                         kickable_radius * 2,
                         kickable_radius * 2 );

    // draw future status
    if ( Options::instance().ballVelCycle() > 0
         && disp->show_.ball_.hasVelocity() )
    {
        drawVelocity( painter );
    }

    if ( Options::instance().showBallTrace() )
    {
        drawTrace( painter );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
BallPainter::drawVelocity( QPainter & painter ) const
{
    const Options & opt = Options::instance();

    if ( opt.antiAliasing() )
    {
        painter.setRenderHint( QPainter::Antialiasing, false );
    }

    const rcss::rcg::ServerParamT & sparam = M_main_data.serverParam();

    DispConstPtr disp = M_main_data.getDispInfo( M_main_data.index() );

    const double bdecay = sparam.ball_decay_;

    Vector2D bpos( disp->show_.ball_.x_,
                   disp->show_.ball_.y_ );
    Vector2D bvel( disp->show_.ball_.vx_,
                   disp->show_.ball_.vy_ );

    QPoint first_point( opt.screenX( bpos.x ),
                        opt.screenY( bpos.y ) );
    QPoint last_point = first_point;

    QPainterPath path;

    const int max_cycle = std::min( 100,  Options::instance().ballVelCycle() );
    for ( int i = 0; i < max_cycle; ++i )
    {
        bpos += bvel;
        bvel *= bdecay;

        QPoint pt( opt.screenX( bpos.x ),
                   opt.screenY( bpos.y ) );
        if ( std::abs( last_point.x() - pt.x() ) < 1
             && std::abs( last_point.y() - pt.y() ) < 1 )
        {
            break;
        }
        last_point = pt;
        path.addEllipse( pt.x() - 1,
                         pt.y() - 1,
                         3,
                         3 );
    }

    path.moveTo( first_point );
    path.lineTo( last_point );

    painter.setPen( M_ball_vel_pen );
    painter.setBrush( Qt::NoBrush );

    painter.drawPath( path );

    if ( opt.antiAliasing() )
    {
        painter.setRenderHint( QPainter::Antialiasing );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
BallPainter::drawTrace( QPainter & painter ) const
{
    const Options & opt = Options::instance();

    const std::size_t first = M_main_data.dispHolder().getIndexOf( opt.ballTraceStart() );
    const std::size_t last = M_main_data.dispHolder().getIndexOf( opt.ballTraceEnd() );
    if ( first >= last )
    {
        return;
    }

    const std::vector< DispPtr > & cont = M_main_data.dispHolder().dispInfoCont();
    if ( cont.empty() )
    {
        return;
    }

    if ( opt.antiAliasing() )
    {
        painter.setRenderHint( QPainter::Antialiasing, false );
    }

    const bool line_trace = opt.lineTrace();

    QPen black_dot_pen( Qt::black );
    black_dot_pen.setStyle( Qt::DotLine );

    painter.setBrush( Qt::NoBrush );

    std::size_t i = first;
    int prev_x = opt.screenX( cont[i]->show_.ball_.x_ );
    int prev_y = opt.screenX( cont[i]->show_.ball_.y_ );
    ++i;
    for ( ; i <= last; ++i )
    {
        switch ( cont[i]->pmode_ ) {
        case rcss::rcg::PM_BeforeKickOff:
        case rcss::rcg::PM_TimeOver:
        case rcss::rcg::PM_KickOff_Left:
        case rcss::rcg::PM_KickOff_Right:
            prev_x = opt.screenX( 0.0 );
            prev_y = opt.screenY( 0.0 );
            continue;
        case rcss::rcg::PM_PlayOn:
        case rcss::rcg::PM_AfterGoal_Left:
        case rcss::rcg::PM_AfterGoal_Right:
            painter.setPen( M_ball_pen );
            break;
        default:
            painter.setPen( black_dot_pen );
            break;
        }

        int ix = opt.screenX( cont[i]->show_.ball_.x_ );
        int iy = opt.screenY( cont[i]->show_.ball_.y_ );

        painter.drawLine( prev_x, prev_y, ix, iy );
        if ( ! line_trace )
        {
            painter.drawEllipse( ix - 2, iy - 2, 5, 5 );
        }
        prev_x = ix;
        prev_y = iy;
    }


    if ( opt.antiAliasing() )
    {
        painter.setRenderHint( QPainter::Antialiasing );
    }
}
