// -*-c++-*-

/*!
  \file scene_builder.cpp
  \brief what the 3D view draws and where. Source File.
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

#include "scene_builder.h"

#include "disp_holder.h"
#include "options.h"
#include "player_style.h"

#include <rcss/rcg/types.h>

#include <algorithm>

namespace {

QVector4D
rgba( const QColor & c,
      const float opacity = 1.0f )
{
    return QVector4D( float( c.redF() ), float( c.greenF() ), float( c.blueF() ), opacity );
}

void
add( std::vector< Instance3D > & list,
     const QMatrix4x4 & model,
     const QVector4D & color )
{
    list.push_back( Instance3D{ model, color } );
}

}

/*-------------------------------------------------------------------*/
void
SceneContent::clear()
{
    cylinders.clear();
    spheres.clear();
    cones.clear();
    discs.clear();
    rings.clear();
    decay_rings.clear();
    quads.clear();
}

namespace SceneBuilder {

/*-------------------------------------------------------------------*/
double
ballRadius( const Options & options,
            const double ball_size )
{
    return options.ballSize() >= 0.01 ? options.ballSize() : ball_size;
}

/*-------------------------------------------------------------------*/
double
ballShadowOpacity( const rcss::rcg::BallT & ball )
{
    const double h = std::max( 0.0f, ball.z_ );
    return 0.45 / ( 1.0 + h / 4.0 );
}

/*-------------------------------------------------------------------*/
void
addGoals( SceneContent & content )
{
    const float r = float( std::max( Options::GOAL_POST_RADIUS, 0.06 ) );
    const float half_width = float( Options::GOAL_HALF_WIDTH );
    const float width = float( Options::GOAL_WIDTH );
    const float depth = float( Options::GOAL_DEPTH );
    const QVector4D white( 1.0f, 1.0f, 1.0f, 1.0f );
    const QVector4D net( 1.0f, 1.0f, 1.0f, NET_OPACITY );

    for ( const float side : { -1.0f, 1.0f } )
    {
        const float line_x = side * float( Options::PITCH_HALF_LENGTH );
        const float back_x = line_x + side * depth;
        const float mid_x = ( line_x + back_x ) / 2.0f;

        // the posts and the crossbar
        for ( const float z : { -half_width, half_width } )
        {
            QMatrix4x4 m;
            m.translate( line_x, GOAL_HEIGHT / 2.0f, z );
            m.scale( r, GOAL_HEIGHT, r );
            add( content.cylinders, m, white );
        }
        {
            QMatrix4x4 m;
            m.translate( line_x, GOAL_HEIGHT, 0.0f );
            m.rotate( 90.0f, 1.0f, 0.0f, 0.0f );
            m.scale( r, width, r );
            add( content.cylinders, m, white );
        }

        // the net: the back, the sides and the roof
        {
            QMatrix4x4 m;
            m.translate( back_x, GOAL_HEIGHT / 2.0f, 0.0f );
            m.rotate( 90.0f, 0.0f, 1.0f, 0.0f );
            m.scale( width, GOAL_HEIGHT, 1.0f );
            add( content.quads, m, net );
        }
        for ( const float z : { -half_width, half_width } )
        {
            QMatrix4x4 m;
            m.translate( mid_x, GOAL_HEIGHT / 2.0f, z );
            m.scale( depth, GOAL_HEIGHT, 1.0f );
            add( content.quads, m, net );
        }
        {
            QMatrix4x4 m;
            m.translate( mid_x, GOAL_HEIGHT, 0.0f );
            m.rotate( 90.0f, 1.0f, 0.0f, 0.0f );
            m.scale( depth, width, 1.0f );
            add( content.quads, m, net );
        }

        // a dark floor, as the 2D view draws the goal
        {
            QMatrix4x4 m;
            m.translate( mid_x, 0.01f, 0.0f );
            m.rotate( -90.0f, 1.0f, 0.0f, 0.0f );
            m.scale( depth, width, 1.0f );
            add( content.quads, m, QVector4D( 0.0f, 0.0f, 0.0f, GOAL_FLOOR_OPACITY ) );
        }
    }
}

/*-------------------------------------------------------------------*/
void
addShow( SceneContent & content,
         const rcss::rcg::ShowInfoT & show,
         const DispHolder & holder,
         const Options & opt )
{
    const rcss::rcg::ServerParamT & sp = holder.serverParam();

    if ( opt.showPlayer() )
    {
        for ( const rcss::rcg::PlayerT & p : show.player_ )
        {
            if ( p.side() == rcss::rcg::NEUTRAL )
            {
                continue;
            }
            const rcss::rcg::PlayerTypeT & type = holder.playerType( p.type_ );
            const PlayerStyle style = playerStyle( p, type, sp, opt );
            const float body_r = float( type.player_size_ );
            const float ground_r = float( opt.playerSize() >= 0.01
                                          ? opt.playerSize()
                                          : type.player_size_ + type.kickable_margin_ + sp.ball_size_ );
            const QVector4D body = rgba( style.body.color() );

            QMatrix4x4 m;
            m.translate( p.x_, BODY_HEIGHT / 2.0f, p.y_ );
            m.scale( body_r, BODY_HEIGHT, body_r );
            add( content.cylinders, m, body );

            m.setToIdentity();
            m.translate( p.x_, HEAD_HEIGHT, p.y_ );
            m.scale( HEAD_RADIUS );
            add( content.spheres, m, body );

            // the circle on the ground, as the outer circle in 2D
            m.setToIdentity();
            m.translate( p.x_, DISC_Y, p.y_ );
            m.scale( ground_r, 1.0f, ground_r );
            add( content.discs, m, rgba( style.brush.color(), DISC_OPACITY ) );

            m.setToIdentity();
            m.translate( p.x_, RING_Y, p.y_ );
            m.scale( ground_r, 1.0f, ground_r );
            if ( style.highlighted() )
            {
                add( content.rings, m, rgba( style.pen.color() ) );
            }
            if ( style.decayed() )
            {
                add( content.decay_rings, m, rgba( style.decay.color() ) );
            }

            // the body direction at the chest, the face direction at the head
            m.setToIdentity();
            m.translate( p.x_, BODY_HEIGHT * 0.7f, p.y_ );
            m.rotate( yaw( p.body_ ), 0.0f, 1.0f, 0.0f );
            m.translate( body_r * 0.8f, 0.0f, 0.0f );
            m.scale( 0.45f, 0.14f, 0.14f );
            add( content.cones, m, QVector4D( 0.0f, 0.0f, 0.0f, 1.0f ) );

            const float face = p.body_ + ( p.hasNeck() ? p.neck_ : 0.0f );
            m.setToIdentity();
            m.translate( p.x_, HEAD_HEIGHT, p.y_ );
            m.rotate( yaw( face ), 0.0f, 1.0f, 0.0f );
            m.translate( HEAD_RADIUS * 0.8f, 0.0f, 0.0f );
            m.scale( 0.3f, 0.09f, 0.09f );
            add( content.cones, m, QVector4D( 1.0f, 0.0f, 0.0f, 1.0f ) );
        }
    }

    if ( opt.showBall() )
    {
        const rcss::rcg::BallT & ball = show.ball_;
        const float r = float( ballRadius( opt, sp.ball_size_ ) );

        QMatrix4x4 m;
        m.translate( ball.x_, std::max( 0.0f, ball.z_ ) + r, ball.y_ );
        m.scale( r );
        add( content.spheres, m, rgba( opt.ballBrush().color() ) );

        // the shadow right below, which tells the height
        m.setToIdentity();
        m.translate( ball.x_, BALL_SHADOW_Y, ball.y_ );
        m.scale( r * 1.15f, 1.0f, r * 1.15f );
        add( content.discs, m, QVector4D( 0.0f, 0.0f, 0.0f, float( ballShadowOpacity( ball ) ) ) );
    }
}

}
