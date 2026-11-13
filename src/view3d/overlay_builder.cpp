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
#include "overlay_math.h"

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

namespace {

QPointF
point_of( const Vector2D & v )
{
    return QPointF( v.x, v.y );
}

//! add the points as lines whose distances go on from the distance
float
add_polyline( std::vector< ColorVertex3D > & list,
              const std::vector< Vector2D > & points,
              const QVector4D & color,
              float distance )
{
    for ( size_t i = 1; i < points.size(); ++i )
    {
        list.push_back( ColorVertex3D{ on_ground( point_of( points[i - 1] ), LINE_Y ), color, distance } );
        distance += float( points[i - 1].dist( points[i] ) );
        list.push_back( ColorVertex3D{ on_ground( point_of( points[i] ), LINE_Y ), color, distance } );
    }
    return distance;
}

//! the outline of a goalie's catchable area, in its order round, the
//! distances going on all the way (for the dashes)
void
add_catch_outline( std::vector< ColorVertex3D > & list,
                   const OverlayMath::CatchOutline & outline,
                   const QColor & color )
{
    const QVector4D c = rgba( color );
    float distance = 0.0f;
    for ( size_t i = 0; i < outline.arcs_.size(); ++i )
    {
        const OverlayMath::Arc & arc = outline.arcs_[i];
        const size_t first = list.size();
        addArc( list, point_of( arc.center_ ), arc.radius_, color, arc.start_, arc.span_ );
        for ( size_t v = first; v < list.size(); ++v )
        {
            list[v].distance += distance;
        }
        distance += float( arc.radius_ * arc.span_ * DEG2RAD );
        if ( i == 0 )
        {
            distance = add_polyline( list, outline.between_, c, distance );
        }
    }
    add_polyline( list, outline.edges_, c, distance );
}

}

/*-------------------------------------------------------------------*/
void
addOverlays( SceneContent & content,
             const rcss::rcg::ShowInfoT & show,
             const DispHolder & holder,
             const Options & opt )
{
    const rcss::rcg::ServerParamT & sp = holder.serverParam();
    const rcss::rcg::BallT & ball = show.ball_;

    if ( opt.showPlayer() )
    {
        for ( const rcss::rcg::PlayerT & p : show.player_ )
        {
            if ( p.side() == rcss::rcg::NEUTRAL
                 || ! p.isAlive() )
            {
                continue;
            }
            const rcss::rcg::PlayerTypeT & type = holder.playerType( p.type_ );
            const QPointF center( p.x_, p.y_ );
            const bool selected = opt.selectedPlayer( p.side(), p.unum_ );

            // the focus point of the selected player
            Vector2D focus;
            if ( opt.showFocusPoint()
                 && selected
                 && p.hasNeck() && p.hasView()
                 && OverlayMath::focusPoint( p, &focus ) )
            {
                const QColor color = opt.focusPointPen().color();
                addLine( content.lines, center, point_of( focus ), color );
                addArc( content.lines, point_of( focus ), opt.focusPointSize(), color );
            }

            // the catch areas of a goalie
            if ( opt.showCatchArea()
                 && p.isGoalie() )
            {
                const bool left = ( p.side() == rcss::rcg::LEFT );
                const OverlayMath::CatchArea area = OverlayMath::catchArea( sp, type );
                add_catch_outline( content.lines,
                                   OverlayMath::catchOutline( sp, p, area.catchable_length_ ),
                                   ( left ? opt.leftGoaliePen() : opt.rightGoaliePen() ).color() );
                if ( area.stretched_ )
                {
                    const QColor stretch = ( left ? opt.leftGoalieStretchPen() : opt.rightGoalieStretchPen() ).color();
                    add_catch_outline( content.dotted_lines, OverlayMath::catchOutline( sp, p, area.max_length_ ), stretch );
                    add_catch_outline( content.dotted_lines, OverlayMath::catchOutline( sp, p, area.min_length_ ), stretch );
                }
            }

            // the tackle area, turned with the body, and its chances
            const OverlayMath::TackleChances chances = ( opt.showTackleArea()
                                                         ? OverlayMath::tackleChances( sp, p, ball )
                                                         : OverlayMath::TackleChances{ false, 0.0, false, 0.0 } );
            if ( chances.any() )
            {
                const QColor color = opt.tacklePen().color();
                auto corner = [&]( const double x, const double y ) {
                    Vector2D v( x, y );
                    v.rotate( p.body_ );
                    return QPointF( p.x_ + v.x, p.y_ + v.y );
                };
                const QPointF corners[4] = {
                    corner( - sp.tackle_back_dist_, - sp.tackle_width_ ),
                    corner( sp.tackle_dist_, - sp.tackle_width_ ),
                    corner( sp.tackle_dist_, sp.tackle_width_ ),
                    corner( - sp.tackle_back_dist_, sp.tackle_width_ ),
                };
                for ( int i = 0; i < 4; ++i )
                {
                    addLine( content.lines, corners[i], corners[( i + 1 ) % 4], color );
                }
                content.texts.push_back( OverlayText{ on_ground( center, LINE_Y ), OverlayMath::tackleText( chances ), color } );
            }

            // where the selected player can kick the ball to
            if ( opt.showKickAccelArea()
                 && selected )
            {
                const OverlayMath::KickAccel kick = OverlayMath::kickAccel( sp, type, p, ball );
                if ( kick.kickable_ )
                {
                    const QColor color = opt.kickAccelPen().color();
                    addLine( content.lines, QPointF( ball.x_, ball.y_ ), point_of( kick.next_ ), color );
                    for ( const OverlayMath::Arc & arc : kick.arcs_ )
                    {
                        addArc( content.lines, point_of( arc.center_ ), arc.radius_, color, arc.start_, arc.span_ );
                    }
                    content.texts.push_back( OverlayText{ on_ground( point_of( kick.next_ ), LINE_Y ),
                                                          QString::asprintf( "MaxAccel=%.3f", kick.max_accel_ ),
                                                          color } );
                }
            }

            // where the player points
            if ( opt.showPointto()
                 && p.isPointing() )
            {
                const QColor color = opt.pointtoPen().color();
                const QPointF q( p.point_x_, p.point_y_ );
                const QPointF d( POINTTO_CROSS, POINTTO_CROSS ), e( POINTTO_CROSS, - POINTTO_CROSS );
                addLine( content.lines, center, q, color );
                addLine( content.lines, q - d, q + d, color );
                addLine( content.lines, q - e, q + e, color );
            }
        }

        // the offside lines across the pitch
        if ( opt.showOffsideLine() )
        {
            const OverlayMath::OffsideLines lines = OverlayMath::offsideLines( show );
            const double half = Options::PITCH_HALF_WIDTH;
            addLine( content.lines, QPointF( lines.left_, - half ), QPointF( lines.left_, half ), opt.leftTeamPen().color() );
            addLine( content.lines, QPointF( lines.right_, - half ), QPointF( lines.right_, half ), opt.rightTeamPen().color() );
        }
    }

    // the ball's path without noise: a line to its end and a dot at each cycle
    if ( opt.showBall()
         && opt.ballVelCycle() > 0 )
    {
        const std::vector< Vector2D > path = OverlayMath::ballPath( sp, ball, opt.ballVelCycle(), BALL_PATH_MIN_STEP );
        if ( ! path.empty() )
        {
            const QColor color = opt.ballVelPen().color();
            addLine( content.lines, QPointF( ball.x_, ball.y_ ), point_of( path.back() ), color );
            for ( const Vector2D & q : path )
            {
                content.points.push_back( ColorVertex3D{ on_ground( point_of( q ), LINE_Y ), rgba( color ), 0.0f } );
            }
        }
    }
}

/*-------------------------------------------------------------------*/
std::vector< PlacedText >
placeTexts( const std::vector< OverlayText > & texts,
            const QMatrix4x4 & view_projection,
            const QSize & size,
            const QFontMetrics & metrics )
{
    std::vector< PlacedText > placed;
    std::vector< QRect > taken; // where the texts placed so far are
    for ( const OverlayText & t : texts )
    {
        // the point on the screen; none behind the camera or off the screen
        const QVector4D clip = view_projection * QVector4D( t.at, 1.0f );
        if ( clip.w() <= 1.0e-6f )
        {
            continue;
        }
        const double sx = ( clip.x() / clip.w() + 1.0 ) / 2.0 * size.width();
        const double sy = ( 1.0 - clip.y() / clip.w() ) / 2.0 * size.height();
        if ( sx < 0.0 || sy < 0.0 || sx >= size.width() || sy >= size.height() )
        {
            continue;
        }

        // beside the point, a line down while it would cover another
        const int width = metrics.horizontalAdvance( t.text );
        QRect rect( int( std::lround( sx ) ) + 10, int( std::lround( sy ) ) + 2, width, metrics.height() );
        while ( std::any_of( taken.begin(), taken.end(), [&]( const QRect & r ) { return r.intersects( rect ); } ) )
        {
            rect.translate( 0, metrics.height() );
        }
        taken.push_back( rect );
        placed.push_back( PlacedText{ QPoint( rect.left(), rect.top() + metrics.ascent() ), t.text, t.color } );
    }
    return placed;
}

}
