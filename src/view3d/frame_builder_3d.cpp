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

    // the players as human figures of the model, but not from the top
    // camera (a map); as cylinders if the model cannot be read
    bool figures = ( opt.playerModel3D() != PlayerModel::Simple
                     && camera != CameraMode::Top
                     && M_figures.load() );
    if ( figures
         && ! M_figures.setModel( opt.playerModel3D() ) )
    {
        if ( ! M_model_failed )
        {
            std::cerr << "(FrameBuilder3D) the figures of " << playerModelName( opt.playerModel3D() ).toStdString()
                      << " are not available: " << M_figures.error().toStdString() << std::endl;
            M_model_failed = true;
        }
        figures = false;
    }
    M_model = ( figures ? M_figures.bodyModel() : PlayerModel::Simple );

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
    if ( ! M_has_show )
    {
        return;
    }

    QElapsedTimer clock;
    clock.start();
    SceneBuilder::addShow( content, M_show, holder, opt, seen(), figures );
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
                                                    M_show.player_[figure.player], sp, ball_radius );
        }
    }
    M_figures.pose( content, M_time );
    M_pose_time = ( content.figures.empty() ? 0.0 : clock.nsecsElapsed() * 1.0e-6 );
    OverlayBuilder::addDirections( content, M_show, holder, opt, seen() );
    OverlayBuilder::addViewAreas( content, M_show, holder, opt, seen() );
    OverlayBuilder::addOverlays( content, M_show, holder, opt, seen() );
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
    s.model = ( content.figures.empty() ? PlayerModel::Simple : M_model );
    s.figures_ready = M_figures.ready();
    s.figure_joints = content.figure_joints;
    s.figure_bodies = content.figure_bodies;
    s.shadows = content.shadows;
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
