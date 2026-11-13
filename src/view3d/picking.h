// -*-c++-*-

/*!
  \file picking.h
  \brief what a click on the 3D view aims at. Header File.
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


#ifndef RCSSMONITOR_PICKING_H
#define RCSSMONITOR_PICKING_H

#include "camera_3d.h"
#include "scene_builder.h"

#include "referee_target.h"

#include <QPointF>
#include <QSizeF>
#include <QVector2D>
#include <QVector3D>

#include <vector>

/*!
  \brief what a click on the 3D view aims at (docs/PLAN_3D-VIEW.md §5.4,
  rcssmonitor-web render3d.ts playerAt(), field-view.ts targetAt()): the
  camera's ray through the point of the screen, and the nearest player it
  meets (the body or the head, as drawn) or the ground. It knows nothing of
  OpenGL, so that the tests can check it.
*/
namespace Picking {

/*!
  \struct Ray
  \brief a half line from the camera, in the world (y up)
*/
struct Ray {
    QVector3D origin; //!< on the near plane of the camera
    QVector3D direction; //!< of length 1
};

/*!
  \brief the camera's ray through a point of the screen
  \param camera the camera that drew the screen
  \param view the size of the view (logical pixels)
  \param point the point in the view (logical pixels, from the top left)
*/
Ray rayAt( const CameraPose & camera,
           const QSizeF & view,
           const QPointF & point );

/*!
  \brief where a ray meets the ground
  \param ray the ray
  \param field set to the point on the field (x, y), if it meets the ground
  \return false if it does not (it goes up or along the ground: above the horizon)
*/
bool groundAt( const Ray & ray,
               QVector2D * field );

/*!
  \brief the player that a ray meets first, its body (a cylinder standing on
  the ground, BODY_HEIGHT high) or its head (a sphere at HEAD_HEIGHT)
  \param ray the ray
  \param picks the players drawn
  \return the index in picks, or -1 if it meets none
*/
int playerAt( const Ray & ray,
              const std::vector< PlayerPick > & picks );

/*!
  \brief what a right click aims at: the player under the point, at its
  feet; else the point on the ground, on the pitch or its margin (the area
  of the ground image); else nothing (above the horizon, or the ground
  farther, too near the horizon to aim at)
  \param camera the camera that drew the screen
  \param view the size of the view (logical pixels)
  \param point the point in the view (logical pixels)
  \param picks the players drawn
*/
RefereeTarget targetAt( const CameraPose & camera,
                        const QSizeF & view,
                        const QPointF & point,
                        const std::vector< PlayerPick > & picks );

}

#endif
