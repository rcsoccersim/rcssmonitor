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

#include "disp_holder.h"
#include "options.h"
#include "vector_2d.h"

#include <rcsslogplayer/types.h>

#include <vector>

/*-------------------------------------------------------------------*/
/*!

*/
BallPainter::BallPainter( const DispHolder & disp_holder )
    : M_disp_holder( disp_holder )
{

}

/*-------------------------------------------------------------------*/
/*!

*/
BallPainter::~BallPainter()
{

}

/*-------------------------------------------------------------------*/
/*!

*/
void
BallPainter::draw( QPainter & painter )
{
    const Options & opt = Options::instance();

    if ( ! opt.showBall() )
    {
        return;
    }

    DispConstPtr disp = M_disp_holder.currentDisp();

    if ( ! disp )
    {
        std::cerr << "no dispinfo" << std::endl;
        return;
    }

    const rcss::rcg::ServerParamT & SP = M_disp_holder.serverParam();

    // decide radius
    const int ball_radius = ( opt.ballSize() >= 0.01
                              ? opt.scale( opt.ballSize() )
                              : std::max( 1, opt.scale( SP.ball_size_ ) ) );
    const int kickable_radius
        = std::max( 1, opt.scale( SP.player_size_
                                  + SP.kickable_margin_
                                  + SP.ball_size_ ) );
    const int ix = opt.screenX( disp->show_.ball_.x_ );
    const int iy = opt.screenY( disp->show_.ball_.y_ );

    // draw ball body
    painter.setPen( Qt::NoPen );
    painter.setBrush( opt.ballBrush() );
    painter.drawEllipse( ix - ball_radius,
                         iy - ball_radius,
                         ball_radius * 2,
                         ball_radius * 2 );

    // draw kickable margin
    painter.setPen( opt.ballPen() );
    painter.setBrush( Qt::NoBrush );
    painter.drawEllipse( ix - kickable_radius,
                         iy - kickable_radius,
                         kickable_radius * 2,
                         kickable_radius * 2 );

    // draw future status
    if ( opt.ballVelCycle() > 0
         && disp->show_.ball_.hasVelocity() )
    {
        drawVelocity( painter );
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

    const rcss::rcg::ServerParamT & SP = M_disp_holder.serverParam();

    DispConstPtr disp = M_disp_holder.currentDisp();

    const double bdecay = SP.ball_decay_;

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

    painter.setPen( opt.ballVelPen() );
    painter.setBrush( Qt::NoBrush );

    painter.drawPath( path );

    if ( opt.antiAliasing() )
    {
        painter.setRenderHint( QPainter::Antialiasing );
    }
}
