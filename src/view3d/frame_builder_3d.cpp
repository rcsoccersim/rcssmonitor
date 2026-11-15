// -*-c++-*-

/*!
  \file frame_builder_3d.cpp
  \brief what the 3D view draws in a frame, apart from the widget. Source File.
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

#include "frame_builder_3d.h"

#include "scenery_3d.h"

#include "ball_aids_3d.h"
#include "figure_actions.h"
#include "overlay_builder.h"
#include "scene_builder.h"
#include "stadium_3d.h"

#include "disp_holder.h"
#include "options.h"

#include <QElapsedTimer>
#include <QMatrix3x3>

#include <algorithm>
#include <iostream>

/*-------------------------------------------------------------------*/
void
FrameBuilder3D::build( const DispHolder & holder,
                       const Options & opt,
                       const Interpolate::Position & at,
                       const CameraMode camera,
                       SceneContent & content )
{
    const DispCont & frames = holder.dispCont();
    const rcss::rcg::ServerParamT & sp = holder.serverParam();
    const double step = std::max( 1, sp.simulator_step_ ) * 1.0e-3; // s
    M_at = at;
    M_camera = camera;
    M_time = ( at.index + at.alpha ) * step;

    // The ends in the team colors (the nets, the boards and the banners)
    const QColor team_colors[2] = { opt.leftTeamBrush().color(), opt.rightTeamBrush().color() };
    content.clear();
    SceneBuilder::addGoals( content, team_colors[0], team_colors[1] );
    Stadium3D::addPanels( content, opt.boards3D(), opt.banners3D(), team_colors[0], team_colors[1] );
    // the sky and the ground beyond (Scenery3D)
    content.sky = opt.sky3D();
    content.realistic_grass = ( opt.grass3D() == GrassMode::Realistic );
    content.stands = opt.stands3D();
    content.ball_pattern = opt.ballPattern3D();
    content.far_ground = Scenery3D::farGroundColor( opt.fieldBrush().color() );
    // the sun's detailed shadows (the shadow map), but not from the top
    // camera: they would crowd the map (§11: 8)
    const bool shadows = ( camera != CameraMode::Top );
    content.shadows = ( shadows && opt.shadows3D() == ShadowMode::Detailed );

    // the players and the ball on the way between the two frames
    M_has_show = ! frames.empty();
    M_show = rcss::rcg::ShowInfoT();
    if ( M_has_show )
    {
        M_show = Interpolate::show( *frames[at.index],
                                    ( at.next != at.index ? frames[at.next].get() : nullptr ),
                                    at.alpha );
    }

    // the ball flying in the air, virtually (BallFlight), in log replay
    // with the option: its height over the way between the two frames
    // (none where the view does not move the ball to the next frame)
    M_flying = false;
    if ( M_has_show
         && opt.ballFlight3D()
         && ! M_live )
    {
        M_flight.update( frames, sp, float( SceneBuilder::ballRadius( opt, sp.ball_size_ ) ) );
        if ( M_flight.active() )
        {
            const bool moves = ( at.next != at.index
                                 && Ball3D::rolls( *frames[at.index], *frames[at.next] ) );
            M_show.ball_.z_ = M_flight.heightAt( at.index, moves ? at.alpha : 0.0 );
            M_flying = true;
        }
    }

    // the players as human figures of the figure pack, but not from the
    // top camera (a map); as cylinders if the pack cannot be read (told
    // once, tried again when the packs are looked for again)
    bool figures = ( opt.figures3D()
                     && camera != CameraMode::Top
                     && M_figures.load() );
    if ( figures )
    {
        const QString & id = opt.playerModel3D();
        const FigurePack * pack = FigurePacks::find( id, opt.figureFolder3D() );
        if ( ! pack
             || ! M_figures.hasPack()
             || M_figures.pack().id != pack->id
             || M_figures.pack().dir != pack->dir )
        {
            const QString attempt = ( pack ? pack->dir : id ) + QLatin1Char( '\n' )
                + QString::number( FigurePacks::generation() );
            if ( attempt != M_pack_failed )
            {
                QString why;
                if ( ! pack ) why = QStringLiteral( "no figure pack \"" ) + id + QLatin1Char( '"' );
                else if ( ! M_figures.setPack( *pack ) ) why = M_figures.error();
                FigurePacks::noteError( id, why ); // none: drawn
                if ( ! why.isEmpty() )
                {
                    std::cerr << "(FrameBuilder3D) the figures of " << id.toStdString()
                              << " are not available: " << why.toStdString() << std::endl;
                    M_pack_failed = attempt;
                }
            }
            figures = ( pack && M_figures.hasPack() && M_figures.pack().dir == pack->dir );
        }
    }
    M_model = ( figures ? M_figures.pack().id : QString::fromLatin1( PLAYER_MODEL_SIMPLE ) );

    // what the cameras follow: the ball, and the selected player in the show
    M_subject = CameraSubject();
    M_subject.ball_x = M_show.ball_.x_;
    M_subject.ball_y = M_show.ball_.y_;
    int selected = -1;
    if ( M_has_show )
    {
        for ( int i = 0; i < rcss::rcg::MAX_PLAYER * 2; ++i )
        {
            const rcss::rcg::PlayerT & p = M_show.player_[i];
            if ( p.side() != rcss::rcg::NEUTRAL
                 && p.isAlive()
                 && opt.selectedPlayer( p.side(), p.unum_ ) )
            {
                selected = i;
                M_subject.has_player = true;
                M_subject.player_x = p.x_;
                M_subject.player_y = p.y_;
                M_subject.player = &M_show.player_[i];
                // as tall as it is drawn: its type, and the scale of the figures
                M_subject.player_scale = SceneBuilder::heightRatio( holder.playerType( p.type_ ), sp )
                    * ( figures ? float( opt.figureScale3D() ) : 1.0f );
            }
        }
    }

    // The player camera draws what its player perceives (its sight),
    // decided on the frame of the cycle, not between frames; at its eyes,
    // not the player itself.
    M_seen = false;
    if ( camera == CameraMode::Player
         && selected >= 0 )
    {
        const PlayerCameraOptions & player_camera = opt.playerCamera();
        const rcss::rcg::ShowInfoT & cycle = frames[at.index]->show_;
        M_perceptions = Perceptions::perceive( player_camera.sight,
                                               cycle,
                                               selected,
                                               holder.playerType( cycle.player_[selected].type_ ),
                                               sp,
                                               player_camera.position == PlayerCameraStyle::Position::Eye );
        M_seen = true;
    }

    M_pose_time = 0.0;
    M_ball_turn = QQuaternion();
    M_trail_points = 0;
    if ( ! M_has_show )
    {
        return;
    }

    // how the ball has rolled, from the log (as the figures' gaits): the
    // same at the same frame however it was reached
    M_spin.update( frames, float( SceneBuilder::ballRadius( opt, sp.ball_size_ ) ) );
    M_ball_turn = M_spin.turnAt( frames, at.index, at.next, at.alpha );

    QElapsedTimer clock;
    clock.start();
    SceneBuilder::addShow( content, M_show, holder, opt, seen(), figures ? &M_figures.cast() : nullptr, M_ball_turn );
    // the simple shadows, an ellipse under each player drawn
    if ( shadows && opt.shadows3D() == ShadowMode::Simple )
    {
        SceneBuilder::addBlobShadows( content );
    }
    // how the figures move and what they do, from the log, and so their
    // poses at the game's time of the frame drawn and the fraction to the
    // next one
    if ( ! content.figures.empty() )
    {
        M_gaits.setStrides( M_figures.strides() );
        M_gaits.update( frames, step );
        const float ball_radius = float( SceneBuilder::ballRadius( opt, sp.ball_size_ ) );
        for ( FigureInstance & figure : content.figures )
        {
            figure.motion = M_gaits.motion( figure.player, at.index, at.next, at.alpha );
            figure.actions = FigureActionLog::read( frames, figure.player, at.index, at.next, at.alpha,
                                                    M_show.player_[figure.player], sp, ball_radius, flight() );
        }
    }
    M_figures.pose( content, M_time );
    M_pose_time = ( content.figures.empty() ? 0.0 : clock.nsecsElapsed() * 1.0e-6 );
    OverlayBuilder::addDirections( content, M_show, holder, opt, seen() );
    OverlayBuilder::addViewAreas( content, M_show, holder, opt, seen() );
    OverlayBuilder::addOverlays( content, M_show, holder, opt, seen() );
}

/*-------------------------------------------------------------------*/
void
FrameBuilder3D::addBallAids( const DispHolder & holder,
                             const Options & opt,
                             const CameraPose & camera,
                             const float height,
                             SceneContent & content )
{
    M_trail_points = 0;
    if ( content.balls.empty() ) return; // not drawn, or not seen

    const QVector3D center = content.balls.front().model.map( QVector3D() );
    const float meters_per_pixel = BallAids3D::metersPerPixel( camera, center, height );
    const QColor color = opt.ballBrush().color();
    if ( opt.ballRing3D() )
    {
        BallAids3D::addRing( content, center, float( opt.ballRingRadius3D() ), meters_per_pixel, color );
    }
    if ( opt.ballTrail3D() )
    {
        const float radius = float( SceneBuilder::ballRadius( opt, holder.serverParam().ball_size_ ) );
        const std::vector< BallAids3D::TrailPoint > points
            = BallAids3D::trailPoints( holder.dispCont(), M_at.index, M_at.next, M_at.alpha,
                                       opt.ballTrailCycles3D(), radius, flight() );
        BallAids3D::addTrail( content, points, camera.position,
                              std::max( BallAids3D::TRAIL_WIDTH * radius, BallAids3D::TRAIL_PIXELS * meters_per_pixel ),
                              opt.ballTrailCycles3D(), color );
        if ( ! content.ball_trail.empty() ) M_trail_points = int( points.size() );
    }
}

/*-------------------------------------------------------------------*/
View3DState
FrameBuilder3D::state( const SceneContent & content,
                       const CameraPose & camera ) const
{
    View3DState s;
    s.drawn = true;
    s.at = M_at;
    s.camera = M_camera;
    s.camera_pose = camera;
    s.players = static_cast< int >( content.picks.size() );
    s.figures = static_cast< int >( content.figures.size() );
    s.cylinders = s.players - s.figures;
    s.labels = static_cast< int >( content.labels.size() );
    s.model = ( content.figures.empty() ? QString::fromLatin1( PLAYER_MODEL_SIMPLE ) : M_model );
    s.figures_ready = M_figures.ready();
    s.figure_joints = content.figure_joints;
    s.figure_bodies = content.figure_bodies;
    s.shadows = content.shadows;
    s.sky = content.sky;
    s.realistic_grass = content.realistic_grass;
    s.stands = content.stands;
    s.ball_pattern = content.ball_pattern;
    s.ball_turn = M_ball_turn;
    s.ball_flight = M_flying;
    s.ball_height = M_show.ball_.z_;
    s.ball_ring = ! content.ball_rings.empty();
    s.ball_ring_width = content.ball_ring_width;
    s.ball_trail = ( content.ball_trail.empty() ? 0 : M_trail_points );
    s.blob_shadows = static_cast< int >( content.blob_shadows.size() );

    for ( const FigureInstance & figure : content.figures )
    {
        const Figures3D::Weights w = Figures3D::weightsOf( figure.actions );
        if ( w.kick > 0.0f ) ++s.kicks;
        if ( w.tackle > 0.0f ) ++s.tackles;
        if ( w.fouled > 0.0f ) ++s.fouled;
        if ( w.caught > 0.0f ) ++s.catches;

        FigureProbe probe;
        probe.player = figure.player;
        std::vector< QMatrix4x4 > world;
        M_figures.worldNodes( figure, M_time, world );
        const HumanoidRig & rig = M_figures.rig( M_figures.bodyOf( figure ) );
        const int knee = rig.node( HumanBone::LeftLowerLeg );
        const int head = rig.node( HumanBone::Head );
        if ( knee >= 0 && knee < static_cast< int >( world.size() ) )
        {
            probe.left_knee = world[knee].column( 3 ).toVector3D();
        }
        if ( head >= 0 && head < static_cast< int >( world.size() ) )
        {
            // the turn of the head's matrix, without its scale
            QMatrix3x3 turn;
            for ( int c = 0; c < 3; ++c )
            {
                const QVector3D axis = world[head].column( c ).toVector3D().normalized();
                turn( 0, c ) = axis.x();
                turn( 1, c ) = axis.y();
                turn( 2, c ) = axis.z();
            }
            probe.head = QQuaternion::fromRotationMatrix( turn ).normalized();
        }
        s.probes.push_back( probe );
    }
    return s;
}
