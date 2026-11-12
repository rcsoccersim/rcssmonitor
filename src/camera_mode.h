// -*-c++-*-

/*!
  \file camera_mode.h
  \brief the cameras of the 3D view. Header File.
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

#ifndef RCSSMONITOR_CAMERA_MODE_H
#define RCSSMONITOR_CAMERA_MODE_H

#include <QString>

#include <array>

/*!
  \enum CameraMode
  \brief the cameras of the 3D view (docs/PLAN_3D-VIEW.md §4.1), in the
  order that E goes through them. Here, with no OpenGL, so that the options
  (camera_3d) know them in a build without the 3D view too. The close-up
  cameras and the player's camera come in Phase 6.
*/
enum class CameraMode {
    Free, //!< moved by the mouse (Cameras3D::orbit(), pan(), dolly())
    Broadcast, //!< from beyond the near touch line, after the ball
    Top, //!< straight above, the whole field as in 2D, no perspective
    GoalLeft, //!< behind the left goal, looking at the ball
    GoalRight, //!< behind the right goal, looking at the ball
};

/*!
  \struct CameraModeInfo
  \brief the name of a camera in the options and on the command line (as
  rcssmonitor-web's), and in the menu
*/
struct CameraModeInfo {
    CameraMode mode;
    const char * name;
    const char * label;
};

//! the cameras in the order of E
inline
const std::array< CameraModeInfo, 5 > &
cameraModes()
{
    static const std::array< CameraModeInfo, 5 > modes = { {
            { CameraMode::Free, "free", "Free" },
            { CameraMode::Broadcast, "broadcast", "Broadcast" },
            { CameraMode::Top, "top", "Top" },
            { CameraMode::GoalLeft, "goal-left", "Behind the Left Goal" },
            { CameraMode::GoalRight, "goal-right", "Behind the Right Goal" },
        } };
    return modes;
}

//! the camera that the 3D view starts with
constexpr CameraMode DEFAULT_CAMERA_MODE = CameraMode::Broadcast;

//! the name of a camera in the options
inline
QString
cameraModeName( const CameraMode mode )
{
    for ( const CameraModeInfo & m : cameraModes() )
    {
        if ( m.mode == mode ) return QString::fromLatin1( m.name );
    }
    return QString();
}

/*!
  \brief the camera of a name in the options.
  \param name the name
  \param mode set to the camera if the name is known
  \return false if the name is unknown (mode is left as it was)
*/
inline
bool
cameraModeFromName( const QString & name,
                    CameraMode * mode )
{
    for ( const CameraModeInfo & m : cameraModes() )
    {
        if ( name == QLatin1String( m.name ) )
        {
            *mode = m.mode;
            return true;
        }
    }
    return false;
}

//! the camera after a camera (E), back to the first after the last
inline
CameraMode
nextCameraMode( const CameraMode mode )
{
    const std::array< CameraModeInfo, 5 > & modes = cameraModes();
    for ( std::size_t i = 0; i < modes.size(); ++i )
    {
        if ( modes[i].mode == mode ) return modes[( i + 1 ) % modes.size()].mode;
    }
    return modes[0].mode;
}

#endif
