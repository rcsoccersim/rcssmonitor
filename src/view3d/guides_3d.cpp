// -*-c++-*-

/*!
  \file guides_3d.cpp
  \brief the compass and the minimap over the 3D view. Source File.
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

#include "guides_3d.h"

#include "ground_texture.h"
#include "options.h"

#include <rcss/rcg/types.h>

#include <QFont>
#include <QPainter>
#include <QPainterPath>

#include <algorithm>
#include <cmath>

namespace Guides3D {

namespace {

constexpr double DEG2RAD = M_PI / 180.0;

float
normalize( const float degree )
{
    float d = std::fmod( degree + 180.0f, 360.0f );
    if ( d < 0.0f ) d += 360.0f;
    return d - 180.0f;
}

//! black or white text, whichever reads better on a color
QColor
text_color_on( const QColor & color )
{
    const auto linear = []( const double s ) { return s <= 0.04045 ? s / 12.92 : std::pow( ( s + 0.055 ) / 1.055, 2.4 ); };
    const double l = 0.2126 * linear( color.redF() ) + 0.7152 * linear( color.greenF() ) + 0.0722 * linear( color.blueF() );
    return ( l > 0.179 ? QColor( Qt::black ) : QColor( Qt::white ) );
}

//! a player's color on the minimap: its team's, its goalie's, black when not connected
QColor
player_color( const rcss::rcg::PlayerT & p,
              const Options & opt )
{
    if ( ! p.isAlive() )
    {
        return Qt::black;
    }
    if ( p.side() == rcss::rcg::LEFT )
    {
        return ( p.isGoalie() ? opt.leftGoalieBrush() : opt.leftTeamBrush() ).color();
    }
    return ( p.isGoalie() ? opt.rightGoalieBrush() : opt.rightTeamBrush() ).color();
}

}

/*-------------------------------------------------------------------*/
bool
shown( const GuideMode mode,
       const CameraMode camera )
{
    return camera != CameraMode::Top
        && ( mode == GuideMode::All
             || ( mode == GuideMode::Player && camera == CameraMode::Player ) );
}

/*-------------------------------------------------------------------*/
CameraGuide
guideOf( const CameraPose & pose,
         const float aspect )
{
    const QVector3D d = pose.target - pose.position;
    const double tan_half = std::tan( pose.fov_y * 0.5 * DEG2RAD ) * aspect;
    return CameraGuide{ pose.position.x(),
                        pose.position.z(),
                        float( std::atan2( d.z(), d.x() ) / DEG2RAD ),
                        float( 2.0 * std::atan( tan_half ) / DEG2RAD ) };
}

/*-------------------------------------------------------------------*/
float
bearing( const QPointF & from,
         const QPointF & to )
{
    return float( std::atan2( to.y() - from.y(), to.x() - from.x() ) / DEG2RAD );
}

/*-------------------------------------------------------------------*/
float
compassOffset( const float direction,
               const float camera_direction )
{
    return normalize( direction - camera_direction ) / 180.0f;
}

/*-------------------------------------------------------------------*/
QRectF
compassRect( const double width )
{
    const double w = std::round( std::min( 480.0, width * 0.6 ) );
    return QRectF( std::round( ( width - w ) / 2.0 ), 6.0, w, 22.0 );
}

/*-------------------------------------------------------------------*/
QRectF
minimapRect( const double width,
             const double height,
             double * scale )
{
    const double w = std::round( std::min( 300.0, std::max( 140.0, width * 0.24 ) ) );
    const double h = std::round( w * GroundTexture::width() / GroundTexture::length() );
    *scale = w / GroundTexture::length();
    return QRectF( width - w - 8.0, height - h - 8.0, w, h );
}

/*-------------------------------------------------------------------*/
QPointF
minimapPoint( const QRectF & rect,
              const double scale,
              const QPointF & field )
{
    return QPointF( rect.x() + rect.width() / 2.0 + field.x() * scale,
                    rect.y() + rect.height() / 2.0 + field.y() * scale );
}

/*-------------------------------------------------------------------*/
CompassMarks
drawCompass( QPainter & painter,
             const QRectF & r,
             const CameraGuide & guide,
             const QPointF * ball,
             const Options & opt )
{
    const double center = r.x() + r.width() / 2.0;
    const auto at = [&]( const float direction ) {
        return center + compassOffset( direction, guide.direction ) * ( r.width() / 2.0 );
    };
    const QPointF camera( guide.x, guide.y );

    painter.save();
    painter.setRenderHint( QPainter::Antialiasing );
    QPainterPath frame;
    frame.addRoundedRect( r, 4.0, 4.0 );
    painter.fillPath( frame, QColor( 0, 0, 0, 128 ) );
    painter.setClipPath( frame );

    // what the camera sees
    const double band = std::min( 360.0f, guide.fov ) / 360.0 * r.width();
    painter.fillRect( QRectF( center - band / 2.0, r.y(), band, r.height() ), QColor( 255, 255, 255, 51 ) );

    // ticks every 15 degrees of the field, longer every 45 and 90
    painter.setPen( QPen( QColor( 255, 255, 255, 191 ), 1.0 ) );
    for ( int d = 0; d < 360; d += 15 )
    {
        const double x = std::round( at( float( d ) ) ) + 0.5;
        const double length = ( d % 90 == 0 ? 9.0 : d % 45 == 0 ? 6.0 : 3.0 );
        painter.drawLine( QPointF( x, r.bottom() ), QPointF( x, r.bottom() - length ) );
    }

    // the camera's direction
    painter.setPen( Qt::NoPen );
    painter.setBrush( Qt::white );
    const QPointF triangle[3] = { QPointF( center - 5.0, r.y() ), QPointF( center + 5.0, r.y() ), QPointF( center, r.y() + 6.0 ) };
    painter.drawPolygon( triangle, 3 );

    CompassMarks marks;
    marks.left = at( bearing( camera, QPointF( -Options::PITCH_HALF_LENGTH, 0.0 ) ) );
    marks.right = at( bearing( camera, QPointF( Options::PITCH_HALF_LENGTH, 0.0 ) ) );
    marks.ball = ( ball ? at( bearing( camera, *ball ) ) : -1.0 );

    // the goals, L and R in the team colors
    const double middle = r.y() + r.height() / 2.0;
    QFont font = painter.font();
    font.setBold( true );
    font.setPixelSize( 11 );
    painter.setFont( font );
    for ( int i = 0; i < 2; ++i )
    {
        const double x = ( i == 0 ? marks.left : marks.right );
        const QColor color = ( i == 0 ? opt.leftTeamBrush() : opt.rightTeamBrush() ).color();
        painter.setPen( QPen( Qt::black, 1.0 ) );
        painter.setBrush( color );
        painter.drawEllipse( QPointF( x, middle ), 8.0, 8.0 );
        painter.setPen( text_color_on( color ) );
        painter.drawText( QRectF( x - 8.0, middle - 8.0, 16.0, 16.0 ), Qt::AlignCenter, i == 0 ? "L" : "R" );
    }
    if ( ball )
    {
        painter.setPen( QPen( Qt::black, 1.0 ) );
        painter.setBrush( opt.ballBrush().color() );
        painter.drawEllipse( QPointF( marks.ball, middle ), 4.0, 4.0 );
    }
    painter.restore();
    return marks;
}

/*-------------------------------------------------------------------*/
void
drawMinimap( QPainter & painter,
             const QRectF & r,
             const double scale,
             const CameraGuide & guide,
             const rcss::rcg::ShowInfoT * show,
             const QImage & field,
             const Options & opt )
{
    painter.save();
    painter.setRenderHint( QPainter::Antialiasing );
    painter.setRenderHint( QPainter::SmoothPixmapTransform );
    painter.setOpacity( 0.9 );
    painter.drawImage( r, field );
    painter.setOpacity( 1.0 );
    painter.setClipRect( r );

    // what the camera sees, to beyond the corners (it may be outside)
    const QPointF c = minimapPoint( r, scale, QPointF( guide.x, guide.y ) );
    const double reach = 4.0 * ( r.width() + r.height() );
    const double half = std::min( 359.0f, guide.fov ) / 2.0;
    QPainterPath view;
    view.moveTo( c );
    // QPainter's angles go counterclockwise on the screen, the field's clockwise
    view.arcTo( QRectF( c.x() - reach, c.y() - reach, 2.0 * reach, 2.0 * reach ), -( guide.direction - half ), -2.0 * half );
    view.closeSubpath();
    painter.setPen( QPen( QColor( 255, 255, 255, 204 ), 1.0 ) );
    painter.setBrush( QColor( 255, 255, 255, 56 ) );
    painter.drawPath( view );

    const auto dot = [&]( const QPointF & p, const double radius, const QColor & fill ) {
        const QPointF q = minimapPoint( r, scale, p );
        painter.setPen( QPen( Qt::black, 1.0 ) );
        painter.setBrush( fill );
        painter.drawEllipse( q, radius, radius );
        return q;
    };
    const double radius = std::max( 2.5, scale * 1.2 );
    if ( show )
    {
        for ( const rcss::rcg::PlayerT & p : show->player_ )
        {
            if ( p.side() == rcss::rcg::NEUTRAL
                 || ! p.isAlive() )
            {
                continue;
            }
            const QPointF q = dot( QPointF( p.x_, p.y_ ), radius, player_color( p, opt ) );
            if ( opt.selectedPlayer( p.side(), p.unum_ ) )
            {
                painter.setPen( QPen( Qt::white, 1.0 ) );
                painter.setBrush( Qt::NoBrush );
                painter.drawEllipse( q, radius + 2.0, radius + 2.0 );
            }
        }
        dot( QPointF( show->ball_.x_, show->ball_.y_ ), std::max( 2.0, radius * 0.7 ), opt.ballBrush().color() );
    }
    // the camera
    dot( QPointF( guide.x, guide.y ), 3.5, Qt::white );
    painter.restore();

    painter.save();
    painter.setPen( QPen( QColor( 0, 0, 0, 153 ), 1.0 ) );
    painter.setBrush( Qt::NoBrush );
    painter.drawRect( r.adjusted( 0.5, 0.5, -0.5, -0.5 ) );
    painter.restore();
}

}
