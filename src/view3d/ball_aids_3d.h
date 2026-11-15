// -*-c++-*-

/*!
  \file ball_aids_3d.h
  \brief what helps to find the ball in the 3D view: a ring and a trail. Header File.
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

#ifndef RCSSMONITOR_BALL_AIDS_3D_H
#define RCSSMONITOR_BALL_AIDS_3D_H

#include "camera_3d.h"
#include "scene_content.h"

#include "disp_holder.h"

#include <QColor>
#include <QVector3D>

#include <vector>

class BallFlight;

/*!
  \brief what helps to find the ball in the 3D view, where it is small
  (docs/PLAN_3D-VIEW.md §4.3, 8-5, rcssmonitor-web ball-aids.ts; the 3D
  tab's Ball group):
  - a ring on the ground around it, 0.7 m by default (the default
    kickable_margin), its line never thinner than RING_PIXELS on the
    screen, over a faint fill (the shader ring.frag draws the line);
  - a trail behind it like a contrail: a ribbon through the ball's centers
    of the last cycles (trailPoints()), facing the camera, dense along its
    middle and clear at its edges, narrower and fainter the older, its head
    never thinner than TRAIL_PIXELS.

  The trail is from the frames of the log alone, so that a paused, stepped
  or rewound view shows the same (§5.2); it breaks where the view does not
  move the ball between two frames (Ball3D::rolls(): a placement, a play
  mode change). The widths depend on the camera, so the ring and the trail
  are added after the camera is known, as the labels.
*/
namespace BallAids3D {

//! the ring's line: this wide (m), or this many pixels where that is thinner
constexpr float RING_WIDTH = 0.06f;
constexpr float RING_PIXELS = 2.0f;
//! how opaque the ring's line and its fill inside
constexpr float RING_OPACITY = 0.9f;
constexpr float RING_FILL = 0.12f;
//! the ring's height above the ground: under the ball's shadow (SceneBuilder::BALL_SHADOW_Y)
constexpr float RING_Y = 0.028f;
//! the trail's head: this many times the ball's radius (it fades to its edges), or this many pixels where that is thinner
constexpr float TRAIL_WIDTH = 3.0f;
constexpr float TRAIL_PIXELS = 6.0f;
//! how opaque the trail is along its middle at its head
constexpr float TRAIL_OPACITY = 0.75f;

/*!
  \struct TrailPoint
  \brief a point of the trail: the ball's center, and how many cycles ago
  it was there
*/
struct TrailPoint {
    QVector3D at; //!< in the world (m)
    float age; //!< cycles
};

/*!
  \brief the ball's centers from the time drawn back cycles: the place
  drawn, the frames before it, and the last one between two frames, so
  that it is exactly cycles long in time. It stops before a frame the view
  does not move the ball from (Ball3D::rolls()), or where there is none.
  \param frames the frames (DispHolder::dispCont())
  \param index the frame drawn
  \param next the frame toward which it moves (== index if none)
  \param alpha the fraction of the way to next
  \param cycles how long the trail is
  \param radius the ball as drawn (its center that high over its height)
  \param flight the heights of the ball flying virtually (BallFlight), if active; else the frames'
  \return from the newest (age 0) to the oldest
*/
std::vector< TrailPoint > trailPoints( const DispCont & frames,
                                       const std::size_t index,
                                       const std::size_t next,
                                       const double alpha,
                                       const int cycles,
                                       const float radius,
                                       const BallFlight * flight = nullptr );

/*!
  \brief meters per pixel of the screen at a point as a camera sees it
  \param camera the camera
  \param at the point (in front of the camera; at least 0.1 m deep)
  \param height the screen's height (pixels)
*/
float metersPerPixel( const CameraPose & camera,
                      const QVector3D & at,
                      const float height );

/*!
  \brief the ring on the ground around a point
  \param content where to add it (ball_rings, ball_ring_radius, ball_ring_width)
  \param center the ball's center (its x and z)
  \param radius the ring's radius (m)
  \param meters_per_pixel there on the screen: the line at least RING_PIXELS wide
  \param color the ball's color
*/
void addRing( SceneContent & content,
              const QVector3D & center,
              const float radius,
              const float meters_per_pixel,
              const QColor & color );

/*!
  \brief the trail as a ribbon of triangles through points (trailPoints()),
  across them facing the eye: head_width wide at the head and as opaque
  as TRAIL_OPACITY along the middle, narrower and fainter to nothing at
  the age of cycles, clear at the edges
  \param content where to add it (ball_trail)
  \param points the trail's points, from the newest
  \param eye where the camera is
  \param head_width the width at the head (m)
  \param cycles how long the trail is
  \param color the ball's color
*/
void addTrail( SceneContent & content,
               const std::vector< TrailPoint > & points,
               const QVector3D & eye,
               const float head_width,
               const int cycles,
               const QColor & color );

}

#endif
