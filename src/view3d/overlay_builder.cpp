// -*-c++-*-

/*!
  \file overlay_builder.cpp
  \brief the overlays on the ground of the 3D view. Source File.
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

#include "overlay_builder.h"

#include "disp_holder.h"
#include "options.h"

#include <rcss/rcg/types.h>

#include <algorithm>
#include <cmath>

namespace {

constexpr double DEG2RAD = M_PI / 180.0;

QVector4D
rgba( const QColor & c,
      const float opacity = 1.0f )
{
    return QVector4D( float( c.redF() ), float( c.greenF() ), float( c.blueF() ), opacity );
}

//! a point of the field on the ground at the height
QVector3D
on_ground( const QPointF & p,
           const float y )
{
    return QVector3D( float( p.x() ), y, float( p.y() ) );
}

//! the point at the field direction (degree) and the distance from the center
QPointF
toward( const QPointF & center,
        const double radius,
        const double direction )
{
    return QPointF( center.x() + radius * std::cos( direction * DEG2RAD ),
                    center.y() + radius * std::sin( direction * DEG2RAD ) );
}

//! the reach of a player: its kickable area
double
kickable_reach( const rcss::rcg::PlayerTypeT & type,
                const rcss::rcg::ServerParamT & sp )
{
    return type.player_size_ + type.kickable_margin_ + sp.ball_size_;
}

}

namespace OverlayBuilder {

/*-------------------------------------------------------------------*/
void
addLine( std::vector< ColorVertex3D > & list,
         const QPointF & a,
         const QPointF & b,
         const QColor & color,
         const float y )
{
    const QVector4D c = rgba( color );
    const float length = float( std::hypot( b.x() - a.x(), b.y() - a.y() ) );
    list.push_back( ColorVertex3D{ on_ground( a, y ), c, 0.0f } );
    list.push_back( ColorVertex3D{ on_ground( b, y ), c, length } );
}

/*-------------------------------------------------------------------*/
void
addArc( std::vector< ColorVertex3D > & list,
        const QPointF & center,
        const double radius,
        const QColor & color,
        const double start,
        const double span,
        const float y )
{
    const QVector4D c = rgba( color );
    const int n = std::max( 4, int( std::ceil( CIRCLE_SEGMENTS * std::fabs( span ) / 360.0 ) ) );
    const float step = float( radius * std::fabs( span ) * DEG2RAD / n );
    QPointF last = toward( center, radius, start );
    for ( int i = 1; i <= n; ++i )
    {
        const QPointF next = toward( center, radius, start + span * i / n );
        list.push_back( ColorVertex3D{ on_ground( last, y ), c, step * ( i - 1 ) } );
        list.push_back( ColorVertex3D{ on_ground( next, y ), c, step * i } );
        last = next;
    }
}

/*-------------------------------------------------------------------*/
void
addDirections( SceneContent & content,
               const rcss::rcg::ShowInfoT & show,
               const DispHolder & holder,
               const Options & opt )
{
    if ( ! opt.showPlayer() )
    {
        return;
    }

    const rcss::rcg::ServerParamT & sp = holder.serverParam();
    for ( const rcss::rcg::PlayerT & p : show.player_ )
    {
        if ( p.side() == rcss::rcg::NEUTRAL
             || ! p.isAlive() )
        {
            continue;
        }
        const QPointF center( p.x_, p.y_ );
        const double reach = kickable_reach( holder.playerType( p.type_ ), sp );
        addLine( content.lines, center, toward( center, reach, p.body_ ), opt.playerPen().color() );
        if ( p.hasNeck()
             && ! opt.showViewArea() )
        {
            addLine( content.lines, center, toward( center, reach, p.body_ + p.neck_ ), opt.neckPen().color() );
        }
    }
}

/*-------------------------------------------------------------------*/
void
addViewAreas( SceneContent & content,
              const rcss::rcg::ShowInfoT & show,
              const DispHolder & holder,
              const Options & opt )
{
    if ( ! opt.showPlayer()
         || ! opt.showViewArea() )
    {
        return;
    }

    const rcss::rcg::ServerParamT & sp = holder.serverParam();
    for ( const rcss::rcg::PlayerT & p : show.player_ )
    {
        if ( p.side() == rcss::rcg::NEUTRAL
             || ! p.isAlive()
             || ! p.hasNeck()
             || ! p.hasView() )
        {
            continue;
        }
        const QPointF center( p.x_, p.y_ );
        const double head = p.body_ + p.neck_;
        const double width = std::max( 1.0f, p.view_width_ );
        const double start = head - width / 2.0;

        if ( opt.selectedPlayer( p.side(), p.unum_ ) )
        {
            // the large view: the arcs, the edges to the farthest arc, the circle that is felt
            const QColor color = opt.largeViewAreaPen().color();
            for ( const double r : LARGE_VIEW_RADII )
            {
                addArc( content.lines, center, r, color, start, width, LARGE_VIEW_Y );
            }
            addLine( content.lines, center, toward( center, LARGE_VIEW_RADII[2], start ), color, LARGE_VIEW_Y );
            addLine( content.lines, center, toward( center, LARGE_VIEW_RADII[2], start + width ), color, LARGE_VIEW_Y );
            addArc( content.lines, center, sp.visible_distance_, color, 0.0, 360.0, LARGE_VIEW_Y );
            continue;
        }

        // a sector to visible_distance, a fan of triangles from the center
        const QVector4D color = rgba( opt.viewAreaPen().color(), SECTOR_OPACITY );
        const QVector3D apex = on_ground( center, SECTOR_Y );
        QVector3D last = on_ground( toward( center, sp.visible_distance_, start ), SECTOR_Y );
        for ( int i = 1; i <= SECTOR_SEGMENTS; ++i )
        {
            const QVector3D next = on_ground( toward( center, sp.visible_distance_, start + width * i / SECTOR_SEGMENTS ), SECTOR_Y );
            content.sectors.push_back( ColorVertex3D{ apex, color, 0.0f } );
            content.sectors.push_back( ColorVertex3D{ last, color, 0.0f } );
            content.sectors.push_back( ColorVertex3D{ next, color, 0.0f } );
            last = next;
        }
    }
}

}
