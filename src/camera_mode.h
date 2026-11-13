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
  (camera_3d) know them in a build without the 3D view too. The player's
  camera comes later in Phase 6.
*/
enum class CameraMode {
    Free, //!< moved by the mouse (Cameras3D::orbit(), pan(), dolly())
    Broadcast, //!< from beyond the near touch line, after the ball
    CloseupBall, //!< near the ball, from the side of the near touch line (Options::closeupDistance3D())
    CloseupPlayer, //!< near the selected player, the ball's without one
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
const std::array< CameraModeInfo, 7 > &
cameraModes()
{
    static const std::array< CameraModeInfo, 7 > modes = { {
            { CameraMode::Free, "free", "Free" },
            { CameraMode::Broadcast, "broadcast", "Broadcast" },
            { CameraMode::CloseupBall, "closeup-ball", "Close-up: Ball" },
            { CameraMode::CloseupPlayer, "closeup-player", "Close-up: Selected Player" },
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
    const std::array< CameraModeInfo, 7 > & modes = cameraModes();
    for ( std::size_t i = 0; i < modes.size(); ++i )
    {
        if ( modes[i].mode == mode ) return modes[( i + 1 ) % modes.size()].mode;
    }
    return modes[0].mode;
}

/*!
  \enum Viewpoint
  \brief the viewpoints of the whole pitch (rcssmonitor-web cameras.ts
  VIEWPOINTS): fixed places of the free camera, each with the whole pitch in
  view (Cameras3D::fitView()). Choosing one puts the free camera there
  (View > 3D Camera, the tool bar, I for the first, Ctrl+I for the next).
*/
enum class Viewpoint {
    Side,
    FarSide,
    LeftEnd,
    RightEnd,
    High,
    NearLeft,
    NearRight,
    FarLeft,
    FarRight,
};

/*!
  \struct ViewpointInfo
  \brief a viewpoint: its name in the menu, and where it looks from
*/
struct ViewpointInfo {
    Viewpoint viewpoint;
    const char * label;
    float azimuth; //!< the field direction (degree) from the center of the pitch: 90 the near touch line (+y, below in 2D), 0 behind the right goal
    float elevation; //!< how high (degree)
};

//! the viewpoints in the order of Ctrl+I; the first is where the free camera starts
inline
const std::array< ViewpointInfo, 9 > &
viewpoints()
{
    static const std::array< ViewpointInfo, 9 > list = { {
            { Viewpoint::Side, "Whole Pitch", 90.0f, 32.0f },
            { Viewpoint::FarSide, "Whole Pitch, Far Side", -90.0f, 32.0f },
            { Viewpoint::LeftEnd, "Whole Pitch, Behind the Left Goal", 180.0f, 35.0f },
            { Viewpoint::RightEnd, "Whole Pitch, Behind the Right Goal", 0.0f, 35.0f },
            { Viewpoint::High, "Whole Pitch, High Above", 90.0f, 62.0f },
            { Viewpoint::NearLeft, "Whole Pitch, Near Left Corner", 135.0f, 35.0f },
            { Viewpoint::NearRight, "Whole Pitch, Near Right Corner", 45.0f, 35.0f },
            { Viewpoint::FarLeft, "Whole Pitch, Far Left Corner", -135.0f, 35.0f },
            { Viewpoint::FarRight, "Whole Pitch, Far Right Corner", -45.0f, 35.0f },
        } };
    return list;
}

//! a viewpoint's information
inline
const ViewpointInfo &
viewpointInfo( const Viewpoint viewpoint )
{
    for ( const ViewpointInfo & v : viewpoints() )
    {
        if ( v.viewpoint == viewpoint ) return v;
    }
    return viewpoints()[0];
}

//! the viewpoint after a viewpoint (Ctrl+I), back to the first after the last
inline
Viewpoint
nextViewpoint( const Viewpoint viewpoint )
{
    const std::array< ViewpointInfo, 9 > & list = viewpoints();
    for ( std::size_t i = 0; i < list.size(); ++i )
    {
        if ( list[i].viewpoint == viewpoint ) return list[( i + 1 ) % list.size()].viewpoint;
    }
    return list[0].viewpoint;
}

//! a close-up camera (CameraMode::CloseupBall, CloseupPlayer)
inline
bool
isCloseup( const CameraMode mode )
{
    return mode == CameraMode::CloseupBall || mode == CameraMode::CloseupPlayer;
}

#endif
