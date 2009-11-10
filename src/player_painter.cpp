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
    , have_full_effort_( std::fabs( player.effort_ - ptype.effort_max_ ) < 0.0001 )
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
    , M_player_font( "Sans Serif", 10 )
    , M_player_pen( QColor( 0, 0, 0 ), 0, Qt::SolidLine )
    , M_selected_player_pen( QColor( 0, 0, 0 ), 2, Qt::SolidLine )
    , M_left_team_pen( QColor( 255, 215, 0 ), 0, Qt::SolidLine )
    , M_left_team_brush( QColor( 255, 215, 0 ), Qt::SolidPattern )
    , M_left_goalie_pen( QColor( 39, 231, 31 ), 0, Qt::SolidLine )
    , M_left_goalie_stretch_pen( QColor( 39, 231, 31 ), 0, Qt::DotLine )
    , M_left_goalie_brush( QColor( 39, 231, 31 ), Qt::SolidPattern )
    , M_right_team_pen( QColor( 0, 224, 224 ), 0, Qt::SolidLine )
      //, M_right_team_brush( QColor( 0, 224, 224 ), Qt::SolidPattern )
    , M_right_team_brush( QColor( 0, 191, 255 ), Qt::SolidPattern )
    , M_right_goalie_pen( QColor( 255, 153, 255 ), 0, Qt::SolidLine )
    , M_right_goalie_stretch_pen( QColor( 255, 153, 255 ), 0, Qt::DotLine )
    , M_right_goalie_brush( QColor( 255, 153, 255 ), Qt::SolidPattern )
    , M_player_number_pen( QColor( 255, 255, 255 ), 0, Qt::SolidLine )
    , M_player_number_inner_pen( QColor( 0, 0, 0 ), 0, Qt::SolidLine )
    , M_neck_pen( QColor( 255, 0, 0 ), 0, Qt::SolidLine )
    , M_view_area_pen( QColor( 191, 239, 191 ), 0, Qt::SolidLine )
    , M_large_view_area_pen( QColor( 255, 255, 255 ), 0, Qt::SolidLine )
    , M_ball_collide_brush( QColor( 255, 0, 0 ), Qt::SolidPattern )
    , M_player_collide_brush( QColor( 105, 155, 235 ), Qt::SolidPattern )
    , M_kick_pen( QColor( 255, 255, 255 ), 2, Qt::SolidLine )
    , M_kick_fault_brush( QColor( 255, 0, 0 ), Qt::SolidPattern )
    , M_kick_accel_pen( QColor( 0, 255, 0 ), 0, Qt::SolidLine )
    , M_catch_brush( QColor( 10, 80, 10 ), Qt::SolidPattern )
    , M_catch_fault_brush( QColor( 10, 80, 150 ), Qt::SolidPattern )
    , M_tackle_pen( QColor( 255, 136, 127 ), 2, Qt::SolidLine )
    , M_tackle_brush( QColor( 255, 136, 127 ), Qt::SolidPattern )
    , M_tackle_fault_brush( QColor( 79, 159, 159 ), Qt::SolidPattern )
    , M_foul_charged_brush( QColor( 0, 127, 0 ), Qt::SolidPattern )
    , M_pointto_pen( QColor( 255, 0, 191 ), 1, Qt::SolidLine )
{
    M_player_font.setPointSize( 9 );
    M_player_font.setBold( true );
    //M_player_font.setStyleHint( QFont::System, QFont::PreferBitmap );
    M_player_font.setBold( true );
    M_player_font.setFixedPitch( true );

    readSettings();
}

/*-------------------------------------------------------------------*/
/*

 */
PlayerPainter::~PlayerPainter()
{
    writeSettings();
}

/*-------------------------------------------------------------------*/
/*

 */
void
PlayerPainter::readSettings()
{
    QSettings settings( Options::CONF_FILE,
                        QSettings::IniFormat );

    settings.beginGroup( "Player" );

    QVariant val;

    val = settings.value( "player_font" );
    if ( val.isValid() ) M_player_font.fromString( val.toString() );

    val = settings.value( "player_pen" );
    if ( val.isValid() ) M_player_pen.setColor( val.toString() );

    val = settings.value( "left_team" );
    if ( val.isValid() )
    {
        M_left_team_pen.setColor( val.toString() );
        M_left_team_brush.setColor( val.toString() );
    }

    val = settings.value( "left_goalie" );
    if ( val.isValid() )
    {
        M_left_goalie_pen.setColor( val.toString() );
        M_left_goalie_stretch_pen.setColor( val.toString() );
        M_left_goalie_brush.setColor( val.toString() );
    }

    val = settings.value( "right_team" );
    if ( val.isValid() )
    {
        M_right_team_pen.setColor( val.toString() );
        M_right_team_brush.setColor( val.toString() );
    }

    val = settings.value( "right_goalie" );
    if ( val.isValid() )
    {
        M_right_goalie_pen.setColor( val.toString() );
        M_right_goalie_stretch_pen.setColor( val.toString() );
        M_right_goalie_brush.setColor( val.toString() );
    }

    val = settings.value( "player_number_pen" );
    if ( val.isValid() ) M_player_number_pen.setColor( val.toString() );

    val = settings.value( "player_number_inner_pen" );
    if ( val.isValid() ) M_player_number_inner_pen.setColor( val.toString() );

    val = settings.value( "neck_pen" );
    if ( val.isValid() ) M_neck_pen.setColor( val.toString() );

    val = settings.value( "view_area_pen" );
    if ( val.isValid() ) M_view_area_pen.setColor( val.toString() );

    val = settings.value( "large_view_area_pen" );
    if ( val.isValid() ) M_large_view_area_pen.setColor( val.toString() );

    val = settings.value( "ball_collide_brush" );
    if ( val.isValid() ) M_ball_collide_brush.setColor( val.toString() );

    val = settings.value( "kick_pen" );
    if ( val.isValid() ) M_kick_pen.setColor( val.toString() );

    val = settings.value( "kick_fault_brush" );
    if ( val.isValid() ) M_kick_fault_brush.setColor( val.toString() );

    val = settings.value( "kick_accel_pen" );
    if ( val.isValid() ) M_kick_accel_pen.setColor( val.toString() );

    val = settings.value( "catch_brush" );
    if ( val.isValid() ) M_catch_brush.setColor( val.toString() );

    val = settings.value( "catch_fault_brush" );
    if ( val.isValid() ) M_catch_fault_brush.setColor( val.toString() );

    val = settings.value( "tackle_pen" );
    if ( val.isValid() ) M_tackle_pen.setColor( val.toString() );

    val = settings.value( "tackle_brush" );
    if ( val.isValid() ) M_tackle_brush.setColor( val.toString() );

    val = settings.value( "tackle_fault_brush" );
    if ( val.isValid() ) M_tackle_fault_brush.setColor( val.toString() );

    val = settings.value( "foul_charged_brush" );
    if ( val.isValid() ) M_foul_charged_brush.setColor( val.toString() );

    settings.endGroup();
}

/*-------------------------------------------------------------------*/
/*

 */
void
PlayerPainter::writeSettings()
{
    QSettings settings( Options::CONF_FILE,
                        QSettings::IniFormat );

    settings.beginGroup( "Player" );

    settings.setValue( "player_font", M_player_font.toString() );

    settings.setValue( "player_pen", M_player_pen.color().name() );
    settings.setValue( "left_team", M_left_team_pen.color().name() );
    settings.setValue( "left_goalie", M_left_goalie_brush.color().name() );
    settings.setValue( "right_team", M_right_team_pen.color().name() );
    settings.setValue( "right_goalie", M_right_goalie_brush.color().name() );

    settings.setValue( "player_number_pen", M_player_number_pen.color().name() );
    settings.setValue( "player_number_inner_pen", M_player_number_inner_pen.color().name() );
//     settings.setValue( "player_stamina_pen", M_player_stamina_pen.color().name() );

    settings.setValue( "neck_pen", M_neck_pen.color().name() );
    settings.setValue( "view_area_pen", M_view_area_pen.color().name() );
    settings.setValue( "large_view_area_pen", M_large_view_area_pen.color().name() );
    settings.setValue( "ball_collide_brush", M_ball_collide_brush.color().name() );
    settings.setValue( "kick_pen", M_kick_pen.color().name() );
    settings.setValue( "kick_fault_brush", M_kick_fault_brush.color().name() );
    settings.setValue( "catch_brush", M_catch_brush.color().name() );
    settings.setValue( "catch_fault_brush", M_catch_fault_brush.color().name() );
    settings.setValue( "tackle_pen", M_tackle_pen.color().name() );
    settings.setValue( "tackle_brush", M_tackle_brush.color().name() );
    settings.setValue( "tackle_fault_brush", M_tackle_fault_brush.color().name() );
    settings.setValue( "foul_charged_brush", M_foul_charged_brush.color().name() );

    settings.endGroup();
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
    // decide base color
    if ( Options::instance().selectedPlayer( param.player_.side(), param.player_.unum_ ) )
    {
        painter.setPen( M_selected_player_pen );
    }
    else
    {
        painter.setPen( M_player_pen );
    }

    switch ( param.player_.side_ ) {
    case 'l':
        if ( param.player_.isGoalie() )
        {
            painter.setBrush( M_left_goalie_brush );
        }
        else
        {
            painter.setBrush( M_left_team_brush );
        }
        break;
    case 'r':
        if ( param.player_.isGoalie() )
        {
            painter.setBrush( M_right_goalie_brush );
        }
        else
        {
            painter.setBrush( M_right_team_brush );
        }
        break;
    case 'n':
        std::cerr << "drawBody neutral unum=" << param.player_.unum_ << std::endl;
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
        painter.setPen( M_kick_pen );
    }
    if ( param.player_.isKickingFault() )
    {
        painter.setBrush( M_kick_fault_brush );
    }
    if ( param.player_.isCatching() )
    {
        painter.setBrush( M_catch_brush );
    }
    if ( param.player_.isCatchingFault() )
    {
        painter.setBrush( M_catch_fault_brush );
    }
    if ( param.player_.isTackling() )
    {
        painter.setPen( M_tackle_pen );
        painter.setBrush( M_tackle_brush );
    }
    if ( param.player_.isTacklingFault() )
    {
        painter.setPen( M_tackle_pen );
        painter.setBrush( M_tackle_fault_brush );
    }
    if ( param.player_.isFoulCharged() )
    {
        painter.setBrush( M_foul_charged_brush );
    }
    if ( param.player_.isCollidedBall() )
    {
        painter.setBrush( M_ball_collide_brush );
    }
    if ( param.player_.isCollidedPlayer() )
    {
        painter.setBrush( M_player_collide_brush );
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
    }

    // draw real body edge
    painter.setPen( M_player_pen );
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
    // body direction line
    double real_r
        = param.player_type_.player_size_
        + param.player_type_.kickable_margin_
        + M_disp_holder.serverParam().ball_size_;
    double body = param.player_.body_ * DEG2RAD;

    int bx = Options::instance().screenX( param.player_.x_ + real_r * std::cos( body ) );
    int by = Options::instance().screenY( param.player_.y_ + real_r * std::sin( body ) );

    painter.setPen( M_player_pen );
    painter.setBrush( Qt::NoBrush );
    painter.drawLine( param.x_, param.y_, bx, by );

    if ( param.player_.hasNeck()
         && ! Options::instance().showViewArea() )
    {
        double head = param.player_.body_ + param.player_.neck_;
        head *= DEG2RAD;

        int nx = Options::instance().screenX( param.player_.x_ + real_r * std::cos( head ) );
        int ny = Options::instance().screenY( param.player_.y_ + real_r * std::sin( head ) );

        painter.setPen( M_neck_pen );
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

        painter.setPen( M_large_view_area_pen );
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
        painter.setPen( M_view_area_pen );
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
    const rcss::rcg::ServerParamT & sparam = M_disp_holder.serverParam();

    //
    // catchable area
    //

    const double catchable_area
        = std::sqrt( std::pow( sparam.catchable_area_w_ * 0.5, 2.0 )
                     + std::pow( sparam.catchable_area_l_, 2.0 ) );
    const int catchable = opt.scale( catchable_area );
    painter.setPen( ( param.player_.side_ == 'l' )
                    ? M_left_goalie_pen
                    : M_right_goalie_pen );
    painter.setBrush( Qt::NoBrush );
    painter.drawEllipse( param.x_ - catchable,
                         param.y_ - catchable,
                         catchable * 2,
                         catchable * 2 );

    const double stretch_catchable_area_l
        = sparam.catchable_area_l_
        * param.player_type_.catchable_area_l_stretch_;
    const double stretch_area
        = std::sqrt( std::pow( sparam.catchable_area_w_ * 0.5, 2.0 )
                     + std::pow( stretch_catchable_area_l, 2.0 ) );
    const int stretch = opt.scale( stretch_area );
    if ( stretch > catchable )
    {
        painter.setPen( ( param.player_.side_ == 'l' )
                        ? M_left_goalie_stretch_pen
                        : M_right_goalie_stretch_pen );
        painter.drawEllipse( param.x_ - stretch,
                             param.y_ - stretch,
                             stretch * 2,
                             stretch * 2 );
    }

    //
    // catch probability
    //

    const double ball_dist = std::sqrt( std::pow( param.player_.x_ - param.ball_.x_, 2 )
                                        + std::pow( param.player_.y_ - param.ball_.y_, 2 ) );
    double catch_prob = sparam.catch_probability_;
    if ( ball_dist > stretch_area )
    {
        // catch_prob = 0.0;
        return;
    }

    if ( ball_dist > catchable_area )
    {
        double x = ball_dist * ( stretch_catchable_area_l / stretch_area );
        catch_prob
            = sparam.catch_probability_
            - sparam.catch_probability_ * ( ( x - sparam.catchable_area_l_ )
                                            / ( stretch_catchable_area_l - sparam.catchable_area_l_ ) );
    }

    int text_radius = std::min( 40, param.draw_radius_ );

    painter.setPen( ( param.player_.side_ == 'l' )
                    ? M_right_goalie_pen
                    : M_left_goalie_pen );
    painter.setFont( M_player_font );
    painter.drawText( param.x_ + text_radius,
                      param.y_ + ( 2 + painter.fontMetrics().ascent() ) * 2,
                      QString( "Catch=%1" ).arg( catch_prob, 0, 'g', 3 ) );
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
    const rcss::rcg::ServerParamT & sparam = M_disp_holder.serverParam();

    Vector2D ppos( param.player_.x_,
                   param.player_.y_ );
    Vector2D bpos( param.ball_.x_,
                   param.ball_.y_ );

    Vector2D player_to_ball = bpos - ppos;
    player_to_ball.rotate( - param.player_.body_ );

    double tackle_dist = ( player_to_ball.x > 0.0
                           ? sparam.tackle_dist_
                           : sparam.tackle_back_dist_ );
    if ( tackle_dist < 1.0e-5 )
    {
        return;
    }

    double tackle_fail_prob = ( std::pow( player_to_ball.absX() / tackle_dist,
                                          sparam.tackle_exponent_ )
                                + std::pow( player_to_ball.absY() / sparam.tackle_width_,
                                            sparam.tackle_exponent_ ) );
    double foul_fail_prob = ( std::pow( player_to_ball.absX() / tackle_dist,
                                        sparam.foul_exponent_ )
                              + std::pow( player_to_ball.absY() / sparam.tackle_width_,
                                          sparam.foul_exponent_ ) );

    if ( tackle_fail_prob < 1.0
         || foul_fail_prob < 1.0 )
    {
        painter.save();
        painter.translate( param.x_, param.y_ );
        painter.rotate( param.player_.body_ );

        painter.setPen( M_tackle_pen );
        painter.setBrush( Qt::NoBrush );

        painter.drawRect( opt.scale( - sparam.tackle_back_dist_ ),
                          opt.scale( - sparam.tackle_width_ ),
                          opt.scale( sparam.tackle_dist_ + sparam.tackle_back_dist_ ),
                          opt.scale( sparam.tackle_width_ * 2.0 ) );
        painter.restore();

        int text_radius = std::min( 40, param.draw_radius_ );

        painter.setFont( M_player_font );
        painter.setPen( M_tackle_pen );

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
    const rcss::rcg::ServerParamT & sparam = M_disp_holder.serverParam();

    Vector2D ppos( param.player_.x_,
                   param.player_.y_ );
    Vector2D bpos( param.ball_.x_,
                   param.ball_.y_ );

    Vector2D player_to_ball = bpos - ppos;
    player_to_ball.rotate( - param.player_.body_ );

    double ball_dist = player_to_ball.r();

    if ( ball_dist > ( param.player_type_.player_size_
                       + param.player_type_.kickable_margin_
                       + sparam.ball_size_ ) )
    {
        return;
    }

    double max_kick_accel
        = sparam.max_power_
        //* sparam.kick_power_rate_
        * param.player_type_.kick_power_rate_
        * ( 1.0 - 0.25 * player_to_ball.th().abs() / 180.0
            - 0.25
            * ( ball_dist - param.player_type_.player_size_ - sparam.ball_size_ )
            / param.player_type_.kickable_margin_ );

    if ( max_kick_accel > sparam.ball_accel_max_ )
    {
        max_kick_accel = sparam.ball_accel_max_;
    }

    Vector2D bnext( bpos.x + param.ball_.vx_,
                    bpos.y + param.ball_.vy_ );

    QPoint bpos_screen( opt.screenX( bpos.x ),
                        opt.screenY( bpos.y ) );
    QPoint bnext_screen( opt.screenX( bnext.x ),
                         opt.screenY( bnext.y ) );
    int max_speed_screen = opt.scale( sparam.ball_speed_max_ );
    int max_kick_accel_screen = opt.scale( max_kick_accel );

    painter.setPen( M_kick_accel_pen );
    painter.setBrush( Qt::NoBrush );

    // draw no noise ball move line
    painter.drawLine( bpos_screen, bnext_screen );

    Circle2D max_speed_circle( bpos, sparam.ball_speed_max_ );
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
    painter.setFont( M_player_font );
    painter.setPen( M_kick_accel_pen );

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
    int ix = Options::instance().screenX( param.player_.point_x_ );
    int iy = Options::instance().screenY( param.player_.point_y_ );

    painter.setPen( M_pointto_pen );
    painter.setBrush( Qt::NoBrush );
    painter.drawLine( param.x_, param.y_,
                      ix, iy );
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

    painter.setFont( M_player_font );

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
            painter.setPen( M_player_number_inner_pen );
        }
        else
        {
            painter.setPen( M_player_number_pen );
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

    painter.setPen( M_left_team_pen );
    painter.drawLine( offside_line_l, top_y,
                      offside_line_l, bottom_y );

    painter.setPen( M_right_team_pen );
    painter.drawLine( offside_line_r, top_y,
                      offside_line_r, bottom_y );
}
