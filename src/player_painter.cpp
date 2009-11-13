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

#include <QtGui>

#include "player_painter.h"

#include "disp_holder.h"
#include "options.h"
#include "circle_2d.h"
#include "vector_2d.h"

#include <rcsslogplayer/types.h>

#include <cstring>
#include <cstdio>
#include <cmath>

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
         && player.hasView()
         && opt.showViewArea() )
    {
        drawViewArea( painter, param );
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

    // decide base color
    if ( opt.selectedPlayer( param.player_.side(), param.player_.unum_ ) )
    {
        painter.setPen( opt.selectedPlayerPen() );
    }
    else
    {
        painter.setPen( opt.playerPen() );
    }

    switch ( param.player_.side_ ) {
    case 'l':
        if ( param.player_.isGoalie() )
        {
            painter.setBrush( opt.leftGoalieBrush() );
        }
        else
        {
            painter.setBrush( opt.leftTeamBrush() );
        }
        break;
    case 'r':
        if ( param.player_.isGoalie() )
        {
            painter.setBrush( opt.rightGoalieBrush() );
        }
        else
        {
            painter.setBrush( opt.rightTeamBrush() );
        }
        break;
    case 'n':
        //std::cerr << "drawBody neutral unum=" << param.player_.unum_ << std::endl;
        painter.setBrush( Qt::black );
        break;
    default:
        painter.setBrush( Qt::black );
        break;
    }


    // decide status color
    if ( ! param.player_.isAlive() )
    {
        painter.setBrush( Qt::black );
    }
    if ( param.player_.isKicking() )
    {
        painter.setPen( opt.kickPen() );
    }
    if ( param.player_.isKickingFault() )
    {
        painter.setBrush( opt.kickFaultBrush() );
    }
    if ( param.player_.isCatching() )
    {
        painter.setBrush( opt.catchBrush() );
    }
    if ( param.player_.isCatchingFault() )
    {
        painter.setBrush( opt.catchFaultBrush() );
    }
    if ( param.player_.isTackling() )
    {
        painter.setPen( opt.tacklePen() );
        painter.setBrush( opt.tackleBrush() );
    }
    if ( param.player_.isTacklingFault() )
    {
        painter.setPen( opt.tacklePen() );
        painter.setBrush( opt.tackleFaultBrush() );
    }
    if ( param.player_.isFoulCharged() )
    {
        painter.setBrush( opt.foulChargedBrush() );
    }
    if ( param.player_.isCollidedBall() )
    {
        painter.setBrush( opt.ballCollideBrush() );
    }
    if ( param.player_.isCollidedPlayer() )
    {
        painter.setBrush( opt.playerCollideBrush() );
    }

    painter.drawEllipse( param.x_ - param.draw_radius_ ,
                         param.y_ - param.draw_radius_ ,
                         param.draw_radius_ * 2 ,
                         param.draw_radius_ * 2 );

    // draw stamina status if effort or recovery is decayed.
    if ( param.player_.hasStamina() )
    {
#if QT_VERSION >= 0x040300
        double stamina_rate = param.player_.stamina_ / M_disp_holder.serverParam().stamina_max_;
        int dark_rate = 200 - static_cast< int >( rint( 200 * rint( stamina_rate / 0.125 ) * 0.125 ) );
        dark_rate = std::max( 0, dark_rate - 50 );

        painter.setPen( Qt::NoPen );
        if ( dark_rate == 0 )
        {
            painter.setBrush( painter.brush() );
        }
        else
        {
            painter.setBrush( painter.brush().color().darker( 100 + dark_rate ) );
        }
        painter.drawEllipse( param.x_ - param.body_radius_,
                             param.y_ - param.body_radius_,
                             param.body_radius_ * 2 ,
                             param.body_radius_ * 2  );
#else
        if ( param.player_.stamina_ < 1500.0f )
        {
            painter.setPen( Qt::NoPen );
            painter.setBrush( Qt::black );
            painter.drawEllipse( param.x_ - param.body_radius_,
                                 param.y_ - param.body_radius_,
                                 param.body_radius_ * 2 ,
                                 param.body_radius_ * 2  );
        }
#endif

        if ( std::fabs( param.player_.effort_ - param.player_type_.effort_max_ ) > 1.0e-4 )
        {
            int r = param.draw_radius_ + 2;
            painter.setPen( opt.effortDecayedPen() );
            painter.setBrush( Qt::NoBrush );
            painter.drawEllipse( param.x_ - r, param.y_ - r, r * 2, r * 2 );
        }
        else if ( std::fabs( param.player_.recovery_ - M_disp_holder.serverParam().recover_init_ ) > 1.0e-4 )
        {
            int r = param.draw_radius_ + 2;
            painter.setPen( opt.recoveryDecayedPen() );
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
PlayerPainter::drawCatchArea( QPainter & painter,
                              const PlayerPainter::Param & param ) const
{
    //
    // goalie's catchable area
    //
    const Options & opt = Options::instance();
    const rcss::rcg::ServerParamT & SP = M_disp_holder.serverParam();

    //
    // catchable area
    //

    const double catchable_area
        = std::sqrt( std::pow( SP.catchable_area_w_ * 0.5, 2.0 )
                     + std::pow( SP.catchable_area_l_, 2.0 ) );
    const int catchable = opt.scale( catchable_area );
    painter.setPen( ( param.player_.side_ == 'l' )
                    ? opt.leftGoaliePen()
                    : opt.rightGoaliePen() );
    painter.setBrush( Qt::NoBrush );
    painter.drawEllipse( param.x_ - catchable,
                         param.y_ - catchable,
                         catchable * 2,
                         catchable * 2 );

    const double max_catchable_area_l
        = SP.catchable_area_l_
        * param.player_type_.catchable_area_l_stretch_;
    const double max_area
        = std::sqrt( std::pow( SP.catchable_area_w_ * 0.5, 2.0 )
                     + std::pow( max_catchable_area_l, 2.0 ) );
    const int max_r = opt.scale( max_area );
    if ( max_r > catchable )
    {
        painter.setPen( ( param.player_.side_ == 'l' )
                        ? opt.leftGoalieStretchPen()
                        : opt.rightGoalieStretchPen() );
        painter.drawEllipse( param.x_ - max_r,
                             param.y_ - max_r,
                             max_r * 2,
                             max_r * 2 );

        const double min_catchable_area_l
            = SP.catchable_area_l_
            * ( 1.0 - ( param.player_type_.catchable_area_l_stretch_ - 1.0 ) );
        const double min_area
            = std::sqrt( std::pow( SP.catchable_area_w_ * 0.5, 2.0 )
                         + std::pow( min_catchable_area_l, 2.0 ) );
        const int min_r = opt.scale( min_area );
        painter.drawEllipse( param.x_ - min_r,
                             param.y_ - min_r,
                             min_r * 2,
                             min_r * 2 );
    }

#if 0
    //
    // catch probability
    //

    const double ball_dist = std::sqrt( std::pow( param.player_.x_ - param.ball_.x_, 2 )
                                        + std::pow( param.player_.y_ - param.ball_.y_, 2 ) );
    double catch_prob = SP.catch_probability_;
    if ( ball_dist > stretch_area )
    {
        // catch_prob = 0.0;
        return;
    }

    if ( ball_dist > catchable_area )
    {
        double x = ball_dist * ( stretch_catchable_area_l / stretch_area );
        catch_prob
            = SP.catch_probability_
            - SP.catch_probability_ * ( ( x - SP.catchable_area_l_ )
                                        / ( stretch_catchable_area_l - SP.catchable_area_l_ ) );
    }

    int text_radius = std::min( 40, param.draw_radius_ );

    painter.setPen( ( param.player_.side_ == 'l' )
                    ? opt.rightGoaliePen()
                    : opt.leftGoaliePen() );
    painter.setFont( opt.playerFont() );
    painter.drawText( param.x_ + text_radius,
                      param.y_ + ( 2 + painter.fontMetrics().ascent() ) * 2,
                      QString( "Catch=%1" ).arg( catch_prob, 0, 'g', 3 ) );
#endif
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

    Vector2D ppos( param.player_.x_,
                   param.player_.y_ );
    Vector2D bpos( param.ball_.x_,
                   param.ball_.y_ );

    Vector2D player_to_ball = bpos - ppos;
    player_to_ball.rotate( - param.player_.body_ );

    double tackle_dist = ( player_to_ball.x > 0.0
                           ? SP.tackle_dist_
                           : SP.tackle_back_dist_ );
    if ( tackle_dist < 1.0e-5 )
    {
        return;
    }

    double tackle_fail_prob = ( std::pow( player_to_ball.absX() / tackle_dist,
                                          SP.tackle_exponent_ )
                                + std::pow( player_to_ball.absY() / SP.tackle_width_,
                                            SP.tackle_exponent_ ) );
    double foul_fail_prob = ( std::pow( player_to_ball.absX() / tackle_dist,
                                        SP.foul_exponent_ )
                              + std::pow( player_to_ball.absY() / SP.tackle_width_,
                                          SP.foul_exponent_ ) );

    if ( tackle_fail_prob < 1.0
         || foul_fail_prob < 1.0 )
    {
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

        if ( tackle_fail_prob < 1.0
             && foul_fail_prob < 1.0 )
        {
            painter.drawText( param.x_ + text_radius,
                              param.y_ + 2 + painter.fontMetrics().ascent(),
                              QString( "T=%1,F=%2" )
                              .arg( 1.0 - tackle_fail_prob, 0, 'g', 3 )
                              .arg( 1.0 - foul_fail_prob, 0, 'g', 3 ) );
        }
        else if ( tackle_fail_prob < 1.0 )
        {
            painter.drawText( param.x_ + text_radius,
                              param.y_ + 2 + painter.fontMetrics().ascent(),
                              QString( "Tackle=%1" )
                              .arg( 1.0 - tackle_fail_prob, 0, 'g', 3 ) );
        }
        else if ( foul_fail_prob < 1.0 )
        {
            painter.drawText( param.x_ + text_radius,
                              param.y_ + 2 + painter.fontMetrics().ascent(),
                              QString( "Foul=%1" )
                              .arg( 1.0 - foul_fail_prob, 0, 'g', 3 ) );
        }
    }
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

    if ( ! param.ball_.hasVelocity() )
    {
        return;
    }

    const Options & opt = Options::instance();
    const rcss::rcg::ServerParamT & SP = M_disp_holder.serverParam();

    Vector2D ppos( param.player_.x_,
                   param.player_.y_ );
    Vector2D bpos( param.ball_.x_,
                   param.ball_.y_ );

    Vector2D player_to_ball = bpos - ppos;
    player_to_ball.rotate( - param.player_.body_ );

    double ball_dist = player_to_ball.r();

    if ( ball_dist > ( param.player_type_.player_size_
                       + param.player_type_.kickable_margin_
                       + SP.ball_size_ ) )
    {
        return;
    }

    double max_kick_accel
        = SP.max_power_
        //* SP.kick_power_rate_
        * param.player_type_.kick_power_rate_
        * ( 1.0 - 0.25 * player_to_ball.th().abs() / 180.0
            - 0.25
            * ( ball_dist - param.player_type_.player_size_ - SP.ball_size_ )
            / param.player_type_.kickable_margin_ );

    if ( max_kick_accel > SP.ball_accel_max_ )
    {
        max_kick_accel = SP.ball_accel_max_;
    }

    Vector2D bnext( bpos.x + param.ball_.vx_,
                    bpos.y + param.ball_.vy_ );

    QPoint bpos_screen( opt.screenX( bpos.x ),
                        opt.screenY( bpos.y ) );
    QPoint bnext_screen( opt.screenX( bnext.x ),
                         opt.screenY( bnext.y ) );
    int max_speed_screen = opt.scale( SP.ball_speed_max_ );
    int max_kick_accel_screen = opt.scale( max_kick_accel );

    painter.setPen( opt.kickAccelPen() );
    painter.setBrush( Qt::NoBrush );

    // draw no noise ball move line
    painter.drawLine( bpos_screen, bnext_screen );

    Circle2D max_speed_circle( bpos, SP.ball_speed_max_ );
    Circle2D max_accel_circle( bnext, max_kick_accel );
    Vector2D intersection_1, intersection_2;

    if ( max_speed_circle.intersection( max_accel_circle,
                                        &intersection_1,
                                        &intersection_2 ) != 2 )
    {
        // no intersection points

        // just draw a next ball reachable area by max accel
        painter.drawEllipse( bnext_screen.x() - max_kick_accel_screen,
                             bnext_screen.y() - max_kick_accel_screen,
                             max_kick_accel_screen * 2,
                             max_kick_accel_screen * 2 );
    }
    else
    {
        // exists 2 intersection points

        AngleDeg bpos_to_intersection_1 = ( intersection_1 - bpos ).th();
        AngleDeg bpos_to_intersection_2 = ( intersection_2 - bpos ).th();

        AngleDeg bpos_to_bnext_angle = ( bnext - bpos ).th();

        AngleDeg * bpos_start_angle = 0;
        double bpos_angle_span = 0.0;
        if ( bpos_to_intersection_1.isLeftOf( bpos_to_bnext_angle ) )
        {
            bpos_start_angle = &bpos_to_intersection_1;
            bpos_angle_span = ( bpos_to_intersection_2 - bpos_to_intersection_1 ).degree();
            if ( bpos_angle_span < 0.0 )
            {
                bpos_angle_span += 360.0;
            }
            bpos_angle_span *= -1.0;
        }
        else
        {
            bpos_start_angle = &bpos_to_intersection_2;
            bpos_angle_span = ( bpos_to_intersection_1 - bpos_to_intersection_2 ).degree();
            if ( bpos_angle_span < 0.0 )
            {
                bpos_angle_span += 360.0;
            }
            bpos_angle_span *= -1.0;
        }

        int bpos_start_angle_int = static_cast< int >( rint( - bpos_start_angle->degree() * 16 ) );
        int bpos_angle_span_int = static_cast< int >( rint( bpos_angle_span * 16 ) );
        painter.drawArc( bpos_screen.x() - max_speed_screen,
                         bpos_screen.y() - max_speed_screen,
                         max_speed_screen * 2,
                         max_speed_screen * 2,
                         bpos_start_angle_int,
                         bpos_angle_span_int  );

        AngleDeg bnext_to_intersection_1 = ( intersection_1 - bnext ).th();
        AngleDeg bnext_to_intersection_2 = ( intersection_2 - bnext ).th();

        AngleDeg bnext_to_bpos_angle = bpos_to_bnext_angle + 180.0;

        AngleDeg * bnext_start_angle = 0;
        double bnext_angle_span = 0.0;
        if ( bnext_to_intersection_1.isLeftOf( bnext_to_bpos_angle ) )
        {
            bnext_start_angle = &bnext_to_intersection_1;
            bnext_angle_span = ( bnext_to_intersection_2 - bnext_to_intersection_1 ).degree();
            if ( bnext_angle_span < 0.0 )
            {
                bnext_angle_span += 360.0;
            }
            bnext_angle_span *= -1.0;
        }
        else
        {
            bnext_start_angle = &bnext_to_intersection_2;
            bnext_angle_span = ( bnext_to_intersection_1 - bnext_to_intersection_2 ).degree();
            if ( bnext_angle_span < 0.0 )
            {
                bnext_angle_span += 360.0;
            }
            bnext_angle_span *= -1.0;
        }

        int bnext_start_angle_int = static_cast< int >( rint( - bnext_start_angle->degree() * 16 ) );
        int bnext_angle_span_int = static_cast< int >( rint( bnext_angle_span * 16 ) );
        painter.drawArc( bnext_screen.x() - max_kick_accel_screen,
                         bnext_screen.y() - max_kick_accel_screen,
                         max_kick_accel_screen * 2,
                         max_kick_accel_screen * 2,
                         bnext_start_angle_int,
                         bnext_angle_span_int );
    }

    // draw kick info text
    painter.setFont( opt.playerFont() );
    painter.setPen( opt.kickAccelPen() );

    char buf[32];
    snprintf( buf, 32, "MaxAccel=%.3f", max_kick_accel );
    painter.drawText( bnext_screen.x() + 10,
                      bnext_screen.y() + painter.fontMetrics().ascent(),
                      QString::fromAscii( buf ) );
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

        if ( text_radius < param.draw_radius_ - 10 )
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
                          QString::fromAscii( main_buf ) );
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

    const float ball_x = show.ball_.x_;

    float offside_l = 0.0;
    {
        float min_x = 0.0f;
        for ( int i = 0; i < rcss::rcg::MAX_PLAYER*2; ++i )
        {
            if ( show.player_[i].state_ != 0
                 && show.player_[i].side_ == 'l' )
            {
                float x = show.player_[i].x_;
                if ( x < offside_l )
                {
                    if ( x < min_x )
                    {
                        offside_l = min_x;
                        min_x = x;
                    }
                    else
                    {
                        offside_l = x;
                    }
                }
            }
        }
        offside_l = std::min( offside_l, ball_x );
        offside_l = std::max( offside_l, - float( Options::PITCH_HALF_LENGTH ) );
    }

    float offside_r = 0.0;
    {
        float max_x = 0.0;
        for ( int i = 0; i < rcss::rcg::MAX_PLAYER*2; ++i )
        {
            if ( show.player_[i].state_ != 0
                 && show.player_[i].side_ == 'r' )
            {
                float x = show.player_[i].x_;
                if ( offside_r < x )
                {
                    if ( max_x < x )
                    {
                        offside_r = max_x;
                        max_x = x;
                    }
                    else
                    {
                        offside_r = x;
                    }
                }
            }
        }
        offside_r = std::max( offside_r, ball_x );
        offside_r = std::min( offside_r, float( Options::PITCH_HALF_LENGTH ) );
    }

    const int offside_line_l = opt.screenX( offside_l );
    const int offside_line_r = opt.screenX( offside_r );

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
