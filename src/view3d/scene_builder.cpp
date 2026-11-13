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

#include "label_atlas.h"

#include "disp_holder.h"
#include "options.h"
#include "player_style.h"

#include <rcss/rcg/types.h>

#include <QStringList>

#include <algorithm>
#include <cmath>

namespace {

//! a Qt color as the shaders take it: red, green, blue and opacity, 0 to 1
QVector4D
rgba( const QColor & c,
      const float opacity = 1.0f )
{
    return QVector4D( float( c.redF() ), float( c.greenF() ), float( c.blueF() ), opacity );
}

//! the circle on the ground: the Player Size option, or the kickable area (as in 2D)
float
ground_radius( const rcss::rcg::PlayerTypeT & type,
               const rcss::rcg::ServerParamT & sp,
               const Options & opt )
{
    return float( opt.playerSize() >= 0.01
                  ? opt.playerSize()
                  : type.player_size_ + type.kickable_margin_ + sp.ball_size_ );
}

//! a value within 0 and 1
double
clamp01( const double v )
{
    return std::min( 1.0, std::max( 0.0, v ) );
}

//! the label of a player: its number and its type, as the 2D view writes them (not the stamina)
QString
label_text( const rcss::rcg::PlayerT & p,
            const Options & opt )
{
    QStringList parts;
    if ( opt.showPlayerNumber() )
    {
        parts << QString::number( p.unum_ );
    }
    if ( opt.showPlayerType() )
    {
        parts << QString( "t%1" ).arg( p.type_ );
    }
    return parts.join( ',' );
}

//! add an instance of a shape to its list
void
add( std::vector< Instance3D > & list,
     const QMatrix4x4 & model,
     const QVector4D & color )
{
    list.push_back( Instance3D{ model, color } );
}

}

namespace SceneBuilder {

/*-------------------------------------------------------------------*/
double
staminaRate( const rcss::rcg::PlayerT & player,
             const rcss::rcg::ServerParamT & sp )
{
    return sp.stamina_max_ > 0.0 ? clamp01( player.stamina_ / sp.stamina_max_ ) : 0.0;
}

/*-------------------------------------------------------------------*/
double
capacityRate( const rcss::rcg::PlayerT & player,
              const rcss::rcg::ServerParamT & sp )
{
    if ( ! player.hasStaminaCapacity()
         || sp.stamina_capacity_ <= 0.0 )
    {
        return -1.0;
    }
    return clamp01( player.stamina_capacity_ / sp.stamina_capacity_ );
}

/*-------------------------------------------------------------------*/
double
staminaHue( const double rate,
            const rcss::rcg::ServerParamT & sp )
{
    const double threshold = std::max( sp.recover_dec_thr_, sp.effort_dec_thr_ );
    return threshold >= 1.0 ? 0.0 : 120.0 * clamp01( ( rate - threshold ) / ( 1.0 - threshold ) );
}

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
addGoals( SceneContent & content,
          const QColor & left_net,
          const QColor & right_net )
{
    const float r = float( std::max( Options::GOAL_POST_RADIUS, 0.06 ) );
    const float half_width = float( Options::GOAL_HALF_WIDTH );
    const float width = float( Options::GOAL_WIDTH );
    const float depth = float( Options::GOAL_DEPTH );
    const QVector4D white( 1.0f, 1.0f, 1.0f, 1.0f );

    for ( const float side : { -1.0f, 1.0f } )
    {
        const float line_x = side * float( Options::PITCH_HALF_LENGTH );
        const float back_x = line_x + side * depth;
        const float mid_x = ( line_x + back_x ) / 2.0f;
        const QVector4D net = rgba( side < 0.0f ? left_net : right_net, NET_OPACITY );

        // The posts and the crossbar, of the unit cylinder (radius 1,
        // height 1 along y, centered). The calls of QMatrix4x4 apply to the
        // shape from the last to the first: scaled to the post, then moved.
        for ( const float z : { -half_width, half_width } )
        {
            QMatrix4x4 m;
            m.translate( line_x, GOAL_HEIGHT / 2.0f, z );
            m.scale( r, GOAL_HEIGHT, r );
            add( content.cylinders, m, white );
        }
        {
            // the crossbar: scaled along y to the width, laid down along z
            // (90 degrees about x), and moved to the top of the posts
            QMatrix4x4 m;
            m.translate( line_x, GOAL_HEIGHT, 0.0f );
            m.rotate( 90.0f, 1.0f, 0.0f, 0.0f );
            m.scale( r, width, r );
            add( content.cylinders, m, white );
        }

        // The net, of the unit quad (1 x 1 on the xy plane, facing z): the
        // back turned to face x (90 degrees about y), the sides as they are,
        // the roof laid flat (90 degrees about x)
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

        // a dark floor, as the 2D view draws the goal: the quad laid flat,
        // facing up, just above the ground (under the circles of the players)
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
         const Options & opt,
         const Perceptions * perceptions )
{
    const rcss::rcg::ServerParamT & sp = holder.serverParam();

    if ( opt.showPlayer() )
    {
        for ( int i = 0; i < rcss::rcg::MAX_PLAYER * 2; ++i )
        {
            const rcss::rcg::PlayerT & p = show.player_[i];
            // as the player camera's player perceives it
            const Perception seen = ( perceptions ? perceptions->of( i ) : PlayerSight::FULL );
            if ( p.side() == rcss::rcg::NEUTRAL
                 || ! seen.seen )
            {
                continue;
            }
            const rcss::rcg::PlayerTypeT & type = holder.playerType( p.type_ );
            const PlayerStyle style = playerStyle( p, type, sp, opt );
            const float body_r = float( type.player_size_ );
            const float ground_r = ground_radius( type, sp, opt );
            const QVector4D body = rgba( fadeTeam( style.body.color(), seen.team ) );

            // the body: the unit cylinder scaled to the player and standing
            // on the ground (its center at half the height)
            QMatrix4x4 m;
            m.translate( p.x_, BODY_HEIGHT / 2.0f, p.y_ );
            m.scale( body_r, BODY_HEIGHT, body_r );
            add( content.cylinders, m, body );
            content.picks.push_back( { p.side(), p.unum_, p.x_, p.y_, body_r } );

            // the head on it
            m.setToIdentity();
            m.translate( p.x_, HEAD_HEIGHT, p.y_ );
            m.scale( HEAD_RADIUS );
            add( content.spheres, m, body );

            // the circle on the ground, as the outer circle in 2D
            m.setToIdentity();
            m.translate( p.x_, DISC_Y, p.y_ );
            m.scale( ground_r, 1.0f, ground_r );
            add( content.discs, m, rgba( fadeTeam( style.brush.color(), seen.team ), DISC_OPACITY ) );

            // the rings around the circle: highlighted (kick, tackle,
            // selected, ...) and decayed (effort, recovery), as in 2D
            m.setToIdentity();
            m.translate( p.x_, RING_Y, p.y_ );
            m.scale( ground_r, 1.0f, ground_r );
            if ( style.highlighted() && seen.known() )
            {
                add( content.rings, m, rgba( style.pen.color() ) );
            }
            if ( style.decayed() && seen.known() )
            {
                add( content.decay_rings, m, rgba( style.decay.color() ) );
            }

            if ( ! seen.facing )
            {
                continue;
            }

            // The body direction at the chest, the face direction at the head:
            // the unit cone (pointing +x) made small, moved out of the body
            // along +x, turned about y to the direction, and moved to the
            // player. yaw() turns the field's angle (from +x toward +y) into
            // a rotation about the world's y.
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

    if ( opt.showBall()
         && ( ! perceptions || perceptions->ball ) )
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

/*-------------------------------------------------------------------*/
void
addLabels( SceneContent & content,
           const rcss::rcg::ShowInfoT & show,
           const DispHolder & holder,
           const Options & opt,
           LabelAtlas & atlas,
           const LabelView & view,
           const Perceptions * perceptions )
{
    if ( ! opt.showPlayer() )
    {
        return;
    }

    const rcss::rcg::ServerParamT & sp = holder.serverParam();
    const CameraPose & camera = view.camera;
    const bool top = ( camera.ortho_half_height > 0.0f );
    const float label_scale = float( LabelAtlas::LABEL_SCALE );
    const float aspect = float( LabelAtlas::LABEL_WIDTH ) / LabelAtlas::LABEL_HEIGHT;
    // the height of a number's text (m), 40 of the 64 pixels of a label
    const float text_height = label_scale * LabelAtlas::LABEL_FONT / LabelAtlas::LABEL_HEIGHT;
    const float most_pixels = LABEL_MAX_FONT * float( view.font_pixels );
    const QRectF white = atlas.uv( LabelAtlas::WHITE_SLOT );

    // the camera: forward for the depth, and the pixels per meter 1 m in
    // front of it (perspective) or anywhere (top)
    const QVector3D forward = ( camera.target - camera.position ).normalized();
    const float focal = view.height / ( 2.0f * std::tan( camera.fov_y * float( M_PI ) / 360.0f ) );
    const float top_pixels_per_meter = ( top ? view.height / ( 2.0f * camera.ortho_half_height ) : 0.0f );

    // the labels of each player (a number and a card), to sort by depth
    struct Labels {
        float depth;
        std::vector< Billboard3D > billboards;
    };
    std::vector< Labels > all;

    for ( int i = 0; i < rcss::rcg::MAX_PLAYER * 2; ++i )
    {
        const rcss::rcg::PlayerT & p = show.player_[i];
        const Perception seen = ( perceptions ? perceptions->of( i ) : PlayerSight::FULL );
        if ( p.side() == rcss::rcg::NEUTRAL
             || p.unum_ < 1 || rcss::rcg::MAX_PLAYER < p.unum_
             || ! seen.seen )
        {
            continue;
        }
        const int slot = ( p.side() == rcss::rcg::RIGHT ? rcss::rcg::MAX_PLAYER : 0 ) + p.unum_ - 1;
        const QString text = label_text( p, opt );
        // the number as likely as the player camera's player knows it
        const bool shown = ( ! text.isEmpty() && seen.unum > 0.0f );
        const bool carded = ( opt.showCard() && ( p.hasRedCard() || p.hasYellowCard() ) && seen.known() );
        // the bars: the rates, -1 for none. The server does not tell the
        // others' stamina: with the player camera's sight, its own only.
        const bool own_bars = ( ! perceptions || ! perceptions->active
                                || opt.selectedPlayer( p.side(), p.unum_ ) );
        const double stamina = ( opt.showStamina() && p.hasStamina() && own_bars ? staminaRate( p, sp ) : -1.0 );
        const double capacity = ( opt.showStaminaCapacity() && own_bars ? capacityRate( p, sp ) : -1.0 );
        const bool barred = ( stamina >= 0.0 || capacity >= 0.0 );
        if ( ! shown && ! carded && ! barred )
        {
            continue;
        }
        if ( shown )
        {
            atlas.setText( slot, text, opt.playerFont() );
        }
        const float text_width = ( shown ? float( atlas.textWidth( slot ) ) : 0.0f );

        // where the number is (its center), and k: how many times its size in
        // meters. With the bars, the number goes up so that they are above the head.
        const float height = NUMBER_HEIGHT + ( barred ? BAR_LIFT : 0.0f );
        QVector3D at( p.x_, height, p.y_ );
        float k = 1.0f;
        float bars_x = 0.0f; // from the number's center to the bars' (m)
        float depth = 0.0f;
        if ( top )
        {
            // as the 2D view: to the right of the circle on the ground, the
            // foot of the text at the player's center, as high as the font
            k = float( view.font_pixels ) * LabelAtlas::LABEL_HEIGHT / LabelAtlas::LABEL_FONT
                / top_pixels_per_meter / label_scale;
            const float ground_r = ground_radius( holder.playerType( p.type_ ), sp, opt );
            const float left = p.x_ + ground_r + LABEL_GAP * k;
            at = QVector3D( left + text_width * k / 2.0f, height, p.y_ - text_height * k / 2.0f );
            // the bars from the same left end as the number
            bars_x = ( BAR_WIDTH / 2.0f + BAR_FRAME - text_width / 2.0f ) * k;
        }
        else
        {
            // above the head; nearer than where its text is LABEL_MAX_FONT
            // times the font high, smaller, toward the head
            depth = QVector3D::dotProduct( at - camera.position, forward );
            const float full_pixels = ( depth > 0.01f ? text_height * focal / depth : 0.0f );
            if ( full_pixels > most_pixels )
            {
                k = most_pixels / full_pixels;
            }
            at.setY( LABEL_BASE + k * ( height - LABEL_BASE ) );
        }

        Labels labels;
        labels.depth = depth;
        if ( shown )
        {
            const QColor color = ( opt.showIllegalDefense() && p.isIllegalDefenseState()
                                   ? opt.illegalDefensePen().color()
                                   : QColor( Qt::white ) );
            labels.billboards.push_back( Billboard3D{ at,
                                                      QVector2D( label_scale * aspect * k, label_scale * k ),
                                                      QVector2D( 0.0f, 0.0f ),
                                                      atlas.uv( slot ),
                                                      rgba( color, seen.unum ) } );
        }
        if ( carded )
        {
            const QColor color = ( p.hasRedCard() ? QColor( Qt::red ) : QColor( Qt::yellow ) );
            labels.billboards.push_back( Billboard3D{ at,
                                                      QVector2D( CARD_WIDTH * k, CARD_HEIGHT * k ),
                                                      QVector2D( ( text_width / 2.0f + CARD_GAP + CARD_WIDTH / 2.0f ) * k, 0.0f ),
                                                      white,
                                                      rgba( color ) } );
        }

        // the bars under the number's text, along the camera's down: each a
        // dark background and the fill from its left end
        float from = ( text_height / 2.0f + BAR_GAP ) * k; // from the number's center to the top of the next bar
        auto add_bar = [&]( const float bar_height, const double rate, const QColor & fill ) {
            const float w = BAR_WIDTH * k, h = bar_height * k, f = BAR_FRAME * k;
            const float y = -( from + h / 2.0f );
            labels.billboards.push_back( Billboard3D{ at, QVector2D( w + 2.0f * f, h + 2.0f * f ), QVector2D( bars_x, y ),
                                                      white, QVector4D( 0.0f, 0.0f, 0.0f, BAR_BACK_OPACITY ) } );
            if ( rate > 0.0 )
            {
                labels.billboards.push_back( Billboard3D{ at, QVector2D( w * float( rate ), h ),
                                                          QVector2D( bars_x - float( 1.0 - rate ) * w / 2.0f, y ),
                                                          white, rgba( fill ) } );
            }
            from += h + BAR_GAP * k;
        };
        if ( stamina >= 0.0 )
        {
            add_bar( BAR_STAMINA, stamina, QColor::fromHslF( staminaHue( stamina, sp ) / 360.0, 0.85, 0.5 ) );
        }
        if ( capacity >= 0.0 )
        {
            add_bar( BAR_CAPACITY, capacity, CAPACITY_COLOR );
        }

        all.push_back( labels );
    }

    // far first: a nearer label is drawn over a farther one
    std::stable_sort( all.begin(), all.end(),
                      []( const Labels & a, const Labels & b ) { return a.depth > b.depth; } );
    for ( const Labels & l : all )
    {
        content.labels.insert( content.labels.end(), l.billboards.begin(), l.billboards.end() );
    }
}

}
