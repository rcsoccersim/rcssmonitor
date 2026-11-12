// -*-c++-*-

/*!
  \file camera_3d.h
  \brief where the camera of the 3D view is and how it projects. Header File.
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

#ifndef RCSSMONITOR_CAMERA_3D_H
#define RCSSMONITOR_CAMERA_3D_H

#include <QMatrix4x4>
#include <QVector3D>

/*!
  \struct CameraPose
  \brief where a camera of the 3D view is, where it looks, and how it
  projects. The world has x along the field's x, z along the field's y, and
  y up (rcssmonitor-web scene-math.ts).
*/
struct CameraPose {
    QVector3D position;
    QVector3D target;
    QVector3D up = QVector3D( 0.0f, 1.0f, 0.0f );
    float fov_y = 35.0f; //!< the vertical field of view (degree) of a perspective camera
    float ortho_half_height = 0.0f; //!< > 0: an orthographic camera that shows this half height (m)

    //! the near and far planes (m)
    static constexpr float NEAR = 0.5f;
    static constexpr float FAR = 4000.0f;

    QMatrix4x4 view() const
      {
          QMatrix4x4 m;
          m.lookAt( position, target, up );
          return m;
      }

    QMatrix4x4 projection( const float aspect ) const
      {
          QMatrix4x4 m;
          if ( ortho_half_height > 0.0f )
          {
              m.ortho( -ortho_half_height * aspect, ortho_half_height * aspect,
                       -ortho_half_height, ortho_half_height,
                       NEAR, FAR );
          }
          else
          {
              m.perspective( fov_y, aspect, NEAR, FAR );
          }
          return m;
      }
};

#endif
