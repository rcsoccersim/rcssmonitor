// -*-c++-*-

/*!
  \file plaeyr_painter.cpp
  \brief player painter class Source File.
*/

/*
 *Copyright:

 Copyright (C) The RoboCup Soccer Server Maintenance Group.
 Hidehisa Akiyama

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

#include <QtWidgets>

#include "player_painter.h"

#include "player_style.h"

#include "disp_holder.h"
#include "options.h"
#include "overlay_math.h"
#include "circle_2d.h"
#include "vector_2d.h"

#include <rcss/rcg/types.h>

#include <cstring>
#include <cstdio>
#include <cmath>
#include <iterator>

namespace {
const double DEG2RAD = M_PI / 180.0;
}

/*-------------------------------------------------------------------*/
/*

 */
inline
PlayerPainter::Param::Param(  const rcss::rcg::PlayerT & player,
                              const rcss::rcg::BallT & ball,
                              const rcss::rcg::ServerParamT & sparam,
                              const rcss::rcg::PlayerTypeT & ptype )
    : x_( Options::instance().screenX( player.x_ ) )
    , y_( Options::instance().screenY( player.y_ ) )
    , body_radius_( Options::instance().scale( ptype.player_size_ ) )
    , kick_radius_( Options::instance().scale( ptype.player_size_ + ptype.kickable_margin_ + sparam.ball_size_ ) )
      //, have_full_effort_( std::fabs( player.effort_ - ptype.effort_max_ ) < 1.0e-3 )
    , player_( player )
    , ball_( ball )
    , player_type_( ptype )
{
    if ( body_radius_ < 1 ) body_radius_ = 1;
    if ( kick_radius_ < 5 ) kick_radius_ = 5;

    draw_radius_ =  ( Options::instance().playerSize() >= 0.01
                      ? Options::instance().scale( Options::instance().playerSize() )
                      : kick_radius_ );
}

/*-------------------------------------------------------------------*/
/*

 */
PlayerPainter::PlayerPainter( const DispHolder & disp_holder )
    : M_disp_holder( disp_holder )
{

}

/*-------------------------------------------------------------------*/
/*

 */
PlayerPainter::~PlayerPainter()
{

}

/*-------------------------------------------------------------------*/
/*

 */
void
PlayerPainter::draw( QPainter & painter )
{
    if ( ! Options::instance().showPlayer() )
    {
        return;
    }

    DispConstPtr disp = M_disp_holder.currentDisp();

    if ( ! disp )
    {
        return;
    }

    const rcss::rcg::BallT & ball = disp->show_.ball_;

    for ( int i = 0; i < rcss::rcg::MAX_PLAYER*2; ++i )
    {
        drawAll( painter, disp->show_.player_[i], ball );
    }

    if ( Options::instance().showOffsideLine() )
    {
        drawOffsideLine( painter, disp->show_ );
    }
}

/*-------------------------------------------------------------------*/
/*

 */
void
PlayerPainter::drawAll( QPainter & painter,
                        const rcss::rcg::PlayerT & player,
                        const rcss::rcg::BallT & ball ) const
{
    const Options & opt = Options::instance();
    const Param param( player,
                       ball,
                       M_disp_holder.serverParam(),
                       M_disp_holder.playerType( player.type_ ) );

    drawBody( painter, param );
    drawDir( painter, param );

    if ( player.hasNeck()
         && player.hasView() )
    {
        if ( opt.showViewArea() )
        {
            drawViewArea( painter, param );
        }

        // the selected player's, or with the option all players'
        const bool selected = opt.selectedPlayer( player.side(), player.unum_ );
        if ( opt.showFocusPoint()
             && player.focusDist() > 1.0e-5
             && ( selected || opt.showAllFocusPoints() ) )
        {
            drawFocusPoint( painter, param, selected );
        }
    }

    if ( player.isGoalie()
         && opt.showCatchArea() )
    {
        drawCatchArea( painter, param );
    }

    if ( opt.showTackleArea() )
    {
        drawTackleArea( painter, param );
    }

    if ( opt.showKickAccelArea()
         && opt.selectedPlayer( player.side(), player.unum_ ) )
    {
        drawKickAccelArea( painter, param );
    }

    if ( player.isPointing()
         && opt.showPointto() )
    {
        drawPointto( painter, param );
    }

    drawText( painter, param );
}

/*-------------------------------------------------------------------*/
/*

 */
void
PlayerPainter::drawBody( QPainter & painter,
                         const PlayerPainter::Param & param ) const
{
    const Options & opt = Options::instance();
    const PlayerStyle style = playerStyle( param.player_, param.player_type_, M_disp_holder.serverParam(), opt );

    painter.setPen( style.pen );
    painter.setBrush( style.brush );
    painter.drawEllipse( param.x_ - param.draw_radius_ ,
                         param.y_ - param.draw_radius_ ,
                         param.draw_radius_ * 2 ,
                         param.draw_radius_ * 2 );

    // draw stamina status if effort or recovery is decayed.
    if ( param.player_.hasStamina() )
    {
        painter.setPen( Qt::NoPen );
        painter.setBrush( style.body );
        painter.drawEllipse( param.x_ - param.body_radius_,
                             param.y_ - param.body_radius_,
                             param.body_radius_ * 2 ,
                             param.body_radius_ * 2  );

        if ( style.decayed() )
        {
            int r = param.draw_radius_ + 2;
            painter.setPen( style.decay );
            painter.setBrush( Qt::NoBrush );
            painter.drawEllipse( param.x_ - r, param.y_ - r, r * 2, r * 2 );
        }
    }

    // draw real body edge
    painter.setPen( opt.playerPen() );
    painter.setBrush( Qt::NoBrush );
    painter.drawEllipse( param.x_ - param.body_radius_,
                         param.y_ - param.body_radius_,
                         param.body_radius_ * 2 ,
                         param.body_radius_ * 2  );

}

/*-------------------------------------------------------------------*/
/*!

 */
void
PlayerPainter::drawDir( QPainter & painter,
                        const PlayerPainter::Param & param ) const
{
    const Options & opt = Options::instance();

    // body direction line
    double real_r
        = param.player_type_.player_size_
        + param.player_type_.kickable_margin_
        + M_disp_holder.serverParam().ball_size_;
    double body = param.player_.body_ * DEG2RAD;

    int bx = opt.screenX( param.player_.x_ + real_r * std::cos( body ) );
    int by = opt.screenY( param.player_.y_ + real_r * std::sin( body ) );

    painter.setPen( opt.playerPen() );
    painter.setBrush( Qt::NoBrush );
    painter.drawLine( param.x_, param.y_, bx, by );

    if ( param.player_.hasNeck()
         && ! opt.showViewArea() )
    {
        double head = param.player_.body_ + param.player_.neck_;
        head *= DEG2RAD;

        int nx = opt.screenX( param.player_.x_ + real_r * std::cos( head ) );
        int ny = opt.screenY( param.player_.y_ + real_r * std::sin( head ) );

        painter.setPen( opt.neckPen() );
        painter.setBrush( Qt::NoBrush );
        painter.drawLine( param.x_, param.y_, nx, ny );
    }

}

/*-------------------------------------------------------------------*/
/*!

 */
void
PlayerPainter::drawViewArea( QPainter & painter,
                             const PlayerPainter::Param & param ) const
{
    const Options & opt = Options::instance();

    const int visible_radius = opt.scale( M_disp_holder.serverParam().visible_distance_ );
    const double head = param.player_.body_ + param.player_.neck_;
    const int view_start_angle = static_cast< int >( rint( ( -head - param.player_.view_width_ * 0.5 ) * 16 ) );
    const int span_angle = static_cast< int >( rint( param.player_.view_width_ * 16 ) );


    if ( opt.selectedPlayer( param.player_.side(),
                             param.player_.unum_ ) )
    {
        // draw large view area
        const int UNUM_FAR = opt.scale( 20.0 );
        const int TEAM_FAR = opt.scale( 40.0 );
        const int TEAM_TOOFAR = opt.scale( 60.0 );

        painter.setPen( opt.largeViewAreaPen() );
        painter.setBrush( Qt::NoBrush );

        painter.drawArc( param.x_ - UNUM_FAR, // left x
                         param.y_ - UNUM_FAR, // toop y
                         UNUM_FAR * 2, // width
                         UNUM_FAR * 2, // height
                         view_start_angle,
                         span_angle );
        painter.drawArc( param.x_ - TEAM_FAR, // left x
                         param.y_ - TEAM_FAR, // toop y
                         TEAM_FAR * 2, // width
                         TEAM_FAR * 2, // height
                         view_start_angle,
                         span_angle );
        // pie, no an arc
        painter.drawArc( param.x_ - TEAM_TOOFAR, // left x
                         param.y_ - TEAM_TOOFAR, // toop y
                         TEAM_TOOFAR * 2, // width
                         TEAM_TOOFAR * 2, // height
                         view_start_angle,
                         span_angle );

        // draw feeling area circle
        painter.drawArc( param.x_ - visible_radius,
                         param.y_ - visible_radius,
                         visible_radius * 2,
                         visible_radius * 2,
                         0,
                         360 * 16 );

        const double view_start_angle_real = ( head - param.player_.view_width_ * 0.5 ) * DEG2RAD;
        const double view_end_angle_real = ( head + param.player_.view_width_ * 0.5 ) * DEG2RAD;;
        // left side view cone end point x
        int lx = param.x_
            + opt.scale( 60.0 * std::cos( view_start_angle_real ) );
        // left side view cone end point y
        int ly = param.y_
            + opt.scale( 60.0  * std::sin( view_start_angle_real ) );
        // right side view cone end point x
        int rx = param.x_
            + opt.scale( 60.0 * std::cos( view_end_angle_real ) );
        // right side view cone end point y
        int ry = param.y_
            + opt.scale( 60.0 * std::sin( view_end_angle_real ) );
        painter.drawLine( lx, ly, param.x_, param.y_ );
        painter.drawLine( rx, ry, param.x_, param.y_ );

    }
    else
    {
        // draw small view area
        painter.setPen( opt.viewAreaPen() );
        painter.setBrush( Qt::NoBrush );

        painter.drawPie( param.x_ - visible_radius,
                         param.y_ - visible_radius,
                         visible_radius * 2,
                         visible_radius * 2,
                         view_start_angle,
                         span_angle );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
PlayerPainter::drawFocusPoint( QPainter & painter,
                               const PlayerPainter::Param & param,
                               const bool selected ) const
{
    const Options & opt = Options::instance();
    using OverlayMath::FocusLook;

    Vector2D focus_point;
    if ( ! OverlayMath::focusPoint( param.player_, &focus_point ) )
    {
        return;
    }

    const QPointF focus( opt.screenX( focus_point.x ), opt.screenY( focus_point.y ) );
    const double direction = ( focus_point - Vector2D( param.player_.x(), param.player_.y() ) ).th().degree();
    const auto screen = [&opt]( const Vector2D & p ) { return QPointF( opt.screenX( p.x ), opt.screenY( p.y ) ); };

    painter.setBrush( Qt::NoBrush );

    if ( ! selected )
    {
        // another player's: a smaller reticle in its team color
        const double radius = opt.focusPointSize() * FocusLook::OTHER_SCALE;
        const QColor color = ( param.player_.side() == rcss::rcg::LEFT
                               ? opt.leftTeamBrush() : opt.rightTeamBrush() ).color();
        painter.setPen( QPen( color, 0 ) );
        const double r = opt.scale( radius );
        painter.drawEllipse( focus, r, r );
        const std::vector< Vector2D > ticks = OverlayMath::focusTicks( focus_point, radius, direction );
        for ( size_t i = 0; i + 1 < ticks.size(); i += 2 )
        {
            painter.drawLine( screen( ticks[i] ), screen( ticks[i + 1] ) );
        }
        return;
    }

    const QColor color = opt.focusPointPen().color();

    // the soft spot under the reticle
    {
        QColor fill = color;
        fill.setAlphaF( FocusLook::SPOT_OPACITY );
        painter.setPen( Qt::NoPen );
        painter.setBrush( fill );
        for ( const double k : FocusLook::SPOT_RADII )
        {
            const double r = opt.scale( opt.focusPointSize() * k );
            painter.drawEllipse( focus, r, r );
        }
        painter.setBrush( Qt::NoBrush );
    }

    // the lines of the same noise, fainter outward
    {
        const std::vector< std::vector< std::vector< Vector2D > > > lines
            = OverlayMath::focusNoiseLines( param.player_type_, param.player_ );
        for ( size_t i = 0; i < lines.size(); ++i )
        {
            QColor c = color;
            c.setAlphaF( FocusLook::NOISE_OPACITY[std::min( i, std::size( FocusLook::NOISE_OPACITY ) - 1 )] );
            painter.setPen( QPen( c, 0 ) );
            for ( const std::vector< Vector2D > & line : lines[i] )
            {
                QPolygonF polygon;
                for ( const Vector2D & p : line )
                {
                    polygon << screen( p );
                }
                painter.drawPolyline( polygon );
            }
        }
    }

    // the line from the player, and the reticle
    painter.setPen( opt.focusPointPen() );
    painter.drawLine( QPointF( param.x_, param.y_ ), focus );
    const double r = opt.scale( opt.focusPointSize() );
    painter.drawEllipse( focus, r, r );
    const std::vector< Vector2D > ticks = OverlayMath::focusTicks( focus_point, opt.focusPointSize(), direction );
    for ( size_t i = 0; i + 1 < ticks.size(); i += 2 )
    {
        painter.drawLine( screen( ticks[i] ), screen( ticks[i + 1] ) );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
PlayerPainter::drawCatchArea( QPainter & painter,
                              const PlayerPainter::Param & param ) const
{
    //
    // goalie's catchable area: the outline of where the catch box reaches.
    // Since rcssserver 17.0.0, the goalie cannot catch behind it
    // (min_catch_angle and max_catch_angle).
    //
    const Options & opt = Options::instance();
    const rcss::rcg::ServerParamT & SP = M_disp_holder.serverParam();
    const OverlayMath::CatchArea area = OverlayMath::catchArea( SP, param.player_type_ );

    auto draw_polyline = [&]( const std::vector< Vector2D > & points ) {
        QPolygon polygon;
        for ( const Vector2D & p : points )
        {
            polygon << QPoint( opt.screenX( p.x ), opt.screenY( p.y ) );
        }
        painter.drawPolyline( polygon );
    };
    auto draw_outline = [&]( const double length ) {
        const OverlayMath::CatchOutline outline = OverlayMath::catchOutline( SP, param.player_, length );
        for ( const OverlayMath::Arc & arc : outline.arcs_ )
        {
            const int r = opt.scale( arc.radius_ );
            if ( outline.circle_ )
            {
                painter.drawEllipse( param.x_ - r,
                                     param.y_ - r,
                                     r * 2,
                                     r * 2 );
                return;
            }
            // Qt's angles go counterclockwise on the screen, the field's clockwise.
            painter.drawArc( param.x_ - r,
                             param.y_ - r,
                             r * 2,
                             r * 2,
                             static_cast< int >( rint( - arc.start_ * 16 ) ),
                             static_cast< int >( rint( - arc.span_ * 16 ) ) );
        }
        draw_polyline( outline.between_ );
        draw_polyline( outline.edges_ );
    };

    painter.setPen( ( param.player_.side() == rcss::rcg::LEFT )
                    ? opt.leftGoaliePen()
                    : opt.rightGoaliePen() );
    painter.setBrush( Qt::NoBrush );
    draw_outline( area.catchable_length_ );

    if ( opt.scale( area.max_ ) > opt.scale( area.catchable_ ) )
    {
        painter.setPen( ( param.player_.side() == rcss::rcg::LEFT )
                        ? opt.leftGoalieStretchPen()
                        : opt.rightGoalieStretchPen() );
        draw_outline( area.max_length_ );
        draw_outline( area.min_length_ );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
PlayerPainter::drawTackleArea( QPainter & painter,
                               const PlayerPainter::Param & param ) const
{
    //
    // draw tackle area & probability
    //

    const Options & opt = Options::instance();
    const rcss::rcg::ServerParamT & SP = M_disp_holder.serverParam();

    const OverlayMath::TackleChances chances = OverlayMath::tackleChances( SP, param.player_, param.ball_ );
    if ( ! chances.any() )
    {
        return;
    }

    painter.save();
    painter.translate( param.x_, param.y_ );
    painter.rotate( param.player_.body_ );

    painter.setPen( opt.tacklePen() );
    painter.setBrush( Qt::NoBrush );

    painter.drawRect( opt.scale( - SP.tackle_back_dist_ ),
                      opt.scale( - SP.tackle_width_ ),
                      opt.scale( SP.tackle_dist_ + SP.tackle_back_dist_ ),
                      opt.scale( SP.tackle_width_ * 2.0 ) );
    painter.restore();

    int text_radius = std::min( 40, param.draw_radius_ );

    painter.setFont( opt.playerFont() );
    painter.setPen( opt.tacklePen() );
    painter.drawText( param.x_ + text_radius,
                      param.y_ + 2 + painter.fontMetrics().ascent(),
                      OverlayMath::tackleText( chances ) );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
PlayerPainter::drawKickAccelArea( QPainter & painter,
                                  const PlayerPainter::Param & param ) const
{
    //
    // draw kick accel area
    //

    const Options & opt = Options::instance();
    const rcss::rcg::ServerParamT & SP = M_disp_holder.serverParam();

    const OverlayMath::KickAccel kick = OverlayMath::kickAccel( SP, param.player_type_, param.player_, param.ball_ );
    if ( ! kick.kickable_ )
    {
        return;
    }

    QPoint bpos_screen( opt.screenX( param.ball_.x_ ),
                        opt.screenY( param.ball_.y_ ) );
    QPoint bnext_screen( opt.screenX( kick.next_.x ),
                         opt.screenY( kick.next_.y ) );

    painter.setPen( opt.kickAccelPen() );
    painter.setBrush( Qt::NoBrush );

    // draw no noise ball move line
    painter.drawLine( bpos_screen, bnext_screen );

    if ( kick.arcs_.size() == 1 )
    {
        // no intersection points: just draw a next ball reachable area by max accel
        const int r = opt.scale( kick.max_accel_ );
        painter.drawEllipse( bnext_screen.x() - r,
                             bnext_screen.y() - r,
                             r * 2,
                             r * 2 );
    }
    else
    {
        // the arcs between the intersection points. Qt's angles go
        // counterclockwise on the screen, the field's clockwise.
        for ( const OverlayMath::Arc & arc : kick.arcs_ )
        {
            const int cx = opt.screenX( arc.center_.x );
            const int cy = opt.screenY( arc.center_.y );
            const int r = opt.scale( arc.radius_ );
            painter.drawArc( cx - r,
                             cy - r,
                             r * 2,
                             r * 2,
                             static_cast< int >( rint( - arc.start_ * 16 ) ),
                             static_cast< int >( rint( - arc.span_ * 16 ) ) );
        }
    }

    // draw kick info text
    painter.setFont( opt.playerFont() );
    painter.setPen( opt.kickAccelPen() );

    char buf[32];
    snprintf( buf, 32, "MaxAccel=%.3f", kick.max_accel_ );
    painter.drawText( bnext_screen.x() + 10,
                      bnext_screen.y() + painter.fontMetrics().ascent(),
                      QString::fromLatin1( buf ) );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
PlayerPainter::drawPointto( QPainter & painter,
                            const PlayerPainter::Param & param ) const
{
    const Options & opt = Options::instance();

    int ix = opt.screenX( param.player_.point_x_ );
    int iy = opt.screenY( param.player_.point_y_ );

    painter.setPen( opt.pointtoPen() );
    painter.setBrush( Qt::NoBrush );
    painter.drawLine( param.x_, param.y_, ix, iy );
    painter.drawLine( ix - 2, iy - 2, ix + 2, iy + 2 );
    painter.drawLine( ix - 2, iy + 2, ix + 2, iy - 2 );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
PlayerPainter::drawText( QPainter & painter,
                         const PlayerPainter::Param & param ) const
{
    const Options & opt = Options::instance();

    char main_buf[64];
    std::memset( main_buf, 0, 64 );

    if ( opt.showPlayerNumber() )
    {
        char buf[8];
        snprintf( buf, 8, "%d", param.player_.unum_ );
        std::strcat( main_buf, buf );
    }

    if ( param.player_.hasStamina()
         && opt.showStamina() )
    {
        char buf[16];
        snprintf( buf, 16, "%4.0f", param.player_.stamina_ );
        if ( main_buf[0] != '\0' ) std::strcat( main_buf, "," );
        std::strcat( main_buf, buf );
    }

    if ( param.player_.hasStaminaCapacity()
         && opt.showStaminaCapacity() )
    {
        char buf[16];
        snprintf( buf, 16, "%.0f", param.player_.stamina_capacity_ );
        if ( main_buf[0] != '\0' )
        {
            if ( opt.showStamina() )
            {
                std::strcat( main_buf, "/" );
            }
            else
            {
                std::strcat( main_buf, "," );
            }
        }
        std::strcat( main_buf, buf );
    }

    if ( opt.showPlayerType() )
    {
        char buf[8];
        snprintf( buf, 8, "t%d", param.player_.type_ );
        if ( main_buf[0] != '\0' ) std::strcat( main_buf, "," );
        strcat( main_buf, buf );
    }

    painter.setFont( opt.playerFont() );

    const int text_radius = std::min( 40, param.draw_radius_ );
    int card_offset = 0;

    if ( opt.showCard()
         && ( param.player_.hasRedCard()
              || param.player_.hasYellowCard() ) )
    {
        QFontMetrics fm = painter.fontMetrics();
        int x_size = std::max( 4, fm.ascent() - 2 );
        int y_size = std::max( 4, fm.ascent() );

        card_offset = x_size + 2;

        if ( opt.antiAliasing() )
        {
            painter.setRenderHint( QPainter::Antialiasing, false );
        }
        painter.setPen( Qt::black );
        painter.setBrush( param.player_.hasRedCard()
                          ? Qt::red
                          : Qt::yellow );
        painter.drawRect( param.x_ + text_radius,
                          param.y_ - y_size,
                          x_size, y_size );
        if ( opt.antiAliasing() )
        {
            painter.setRenderHint( QPainter::Antialiasing, true );
        }
    }

    if ( main_buf[0] != '\0' )
    {
        //painter.setPen( param.player_.side() == rcss::rcg::LEFT
        //                ? M_left_team_pen
        //                : M_right_team_pen );

        if ( param.player_.isIllegalDefenseState()
             && opt.showIllegalDefense() )
        {
            painter.setPen( opt.illegalDefensePen() );
        }
        else if ( text_radius < param.draw_radius_ - 10 )
        {
            painter.setPen( opt.playerNumberInnerPen() );
        }
        else
        {
            painter.setPen( opt.playerNumberPen() );
        }

        painter.setBrush( Qt::NoBrush );

        painter.drawText( param.x_ + text_radius + card_offset,
                          param.y_,
                          QString::fromLatin1( main_buf ) );
        painter.setBackgroundMode( Qt::TransparentMode );
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
PlayerPainter::drawOffsideLine( QPainter & painter,
                                const rcss::rcg::ShowInfoT & show ) const
{
    const Options & opt = Options::instance();

    const OverlayMath::OffsideLines lines = OverlayMath::offsideLines( show );

    const int offside_line_l = opt.screenX( lines.left_ );
    const int offside_line_r = opt.screenX( lines.right_ );

    const int top_y = opt.screenY( Options::PITCH_HALF_WIDTH );
    const int bottom_y = opt.screenY( - Options::PITCH_HALF_WIDTH );

    painter.setBrush( Qt::NoBrush );

    painter.setPen( opt.leftTeamPen() );
    painter.drawLine( offside_line_l, top_y,
                      offside_line_l, bottom_y );

    painter.setPen( opt.rightTeamPen() );
    painter.drawLine( offside_line_r, top_y,
                      offside_line_r, bottom_y );
}
