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
#include <utility>

/*!
  \enum CameraMode
  \brief the cameras of the 3D view (docs/plan/2026-11-11_3d-view.md §4.1), in the
  order that E goes through them. Here, with no OpenGL, so that the options
  (camera_3d) know them in a build without the 3D view too.
*/
enum class CameraMode {
    Free, //!< moved by the mouse (Cameras3D::orbit(), pan(), dolly())
    Broadcast, //!< from beyond the near touch line, after the ball
    CloseupBall, //!< near the ball, from the side of the near touch line (Options::closeupDistance3D())
    CloseupPlayer, //!< near the selected player, the ball's without one
    Top, //!< straight above, the whole field as in 2D, no perspective
    GoalLeft, //!< behind the left goal, looking at the ball
    GoalRight, //!< behind the right goal, looking at the ball
    Player, //!< the selected player's camera (Options::playerCamera()), the broadcast camera without one
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
const std::array< CameraModeInfo, 8 > &
cameraModes()
{
    static const std::array< CameraModeInfo, 8 > modes = { {
            { CameraMode::Free, "free", "Free" },
            { CameraMode::Broadcast, "broadcast", "Broadcast" },
            { CameraMode::CloseupBall, "closeup-ball", "Close-up: Ball" },
            { CameraMode::CloseupPlayer, "closeup-player", "Close-up: Selected Player" },
            { CameraMode::Top, "top", "Top" },
            { CameraMode::GoalLeft, "goal-left", "Behind the Left Goal" },
            { CameraMode::GoalRight, "goal-right", "Behind the Right Goal" },
            { CameraMode::Player, "player", "Selected Player" },
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
    const std::array< CameraModeInfo, 8 > & modes = cameraModes();
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

/*!
  \enum GuideMode
  \brief when a guide over the 3D view (the compass, the minimap) shows:
  never, with the player camera, or with every camera but the top one
  (which is a map already)
*/
enum class GuideMode {
    Off,
    Player,
    All,
};

//! the guide modes, with their names in the options and the menu
inline
const std::array< std::pair< GuideMode, const char * >, 3 > &
guideModes()
{
    static const std::array< std::pair< GuideMode, const char * >, 3 > list = { {
            { GuideMode::Off, "off" },
            { GuideMode::Player, "player" },
            { GuideMode::All, "all" },
        } };
    return list;
}

//! the name of a guide mode in the options
inline
QString
guideModeName( const GuideMode mode )
{
    for ( const auto & m : guideModes() )
    {
        if ( m.first == mode ) return QString::fromLatin1( m.second );
    }
    return QString();
}

/*!
  \brief the guide mode of a name in the options
  \return false if the name is unknown (mode is left as it was)
*/
inline
bool
guideModeFromName( const QString & name,
                   GuideMode * mode )
{
    for ( const auto & m : guideModes() )
    {
        if ( name == QLatin1String( m.second ) )
        {
            *mode = m.first;
            return true;
        }
    }
    return false;
}

/*!
  \brief how the 3D view draws the players (docs/plan/2026-11-11_3d-view.md §5.8,
  §5.9): as cylinders and spheres (PLAYER_MODEL_SIMPLE), or as human
  figures of a figure pack, by its name (its folder's: FigurePacks), the
  bundled ones the Mannequin (PLAYER_MODEL_MANNEQUIN) and the human bodies
  in a uniform (PLAYER_MODEL_HUMAN)
*/
constexpr const char * PLAYER_MODEL_SIMPLE = "simple";
constexpr const char * PLAYER_MODEL_MANNEQUIN = "mannequin";
constexpr const char * PLAYER_MODEL_HUMAN = "human";

/*!
  \brief a player model's name as the options keep it: the cylinders if
  empty, the Mannequin for the name it had before there were more figures
  ("figure"), else as it is (a figure pack's name, whether there is such a
  pack or not)
*/
inline
QString
normalizedPlayerModel( const QString & name )
{
    const QString n = name.trimmed();
    if ( n.isEmpty() ) return QString::fromLatin1( PLAYER_MODEL_SIMPLE );
    if ( n == QLatin1String( "figure" ) ) return QString::fromLatin1( PLAYER_MODEL_MANNEQUIN );
    return n;
}

/*!
  \enum FigureBodies
  \brief which bodies the human figures have, of a figure pack with men's
  and women's (the Human): men and women mixed, men only or women only
  (docs/plan/2026-11-11_3d-view.md §5.8)
*/
enum class FigureBodies {
    Mixed,
    Male,
    Female,
};

//! the bodies of the figures, with their names in the options
inline
const std::array< std::pair< FigureBodies, const char * >, 3 > &
figureBodiesList()
{
    static const std::array< std::pair< FigureBodies, const char * >, 3 > list = { {
            { FigureBodies::Mixed, "mixed" },
            { FigureBodies::Male, "male" },
            { FigureBodies::Female, "female" },
        } };
    return list;
}

//! the name of the bodies in the options
inline
QString
figureBodiesName( const FigureBodies bodies )
{
    for ( const auto & b : figureBodiesList() )
    {
        if ( b.first == bodies ) return QString::fromLatin1( b.second );
    }
    return QString();
}

/*!
  \brief the bodies of a name in the options
  \return false if the name is unknown (bodies is left as it was)
*/
inline
bool
figureBodiesFromName( const QString & name,
                      FigureBodies * bodies )
{
    for ( const auto & b : figureBodiesList() )
    {
        if ( name == QLatin1String( b.second ) )
        {
            *bodies = b.first;
            return true;
        }
    }
    return false;
}

/*!
  \enum GrassMode
  \brief the grass of the 3D view (docs/plan/2026-11-11_3d-view.md 8-2, 8-2b): plain,
  mowed in stripes (lighter and darker, the same from everywhere), or
  realistic (the stripes by the way the camera looks, the blades and
  patches of the grass, and the worn grass: Grass3D)
*/
enum class GrassMode {
    Plain,
    Stripes,
    Realistic,
};

//! the grass's modes, with their names in the options
inline
const std::array< std::pair< GrassMode, const char * >, 3 > &
grassModes()
{
    static const std::array< std::pair< GrassMode, const char * >, 3 > list = { {
            { GrassMode::Plain, "plain" },
            { GrassMode::Stripes, "stripes" },
            { GrassMode::Realistic, "realistic" },
        } };
    return list;
}

//! the name of a mode of the grass in the options
inline
QString
grassModeName( const GrassMode mode )
{
    for ( const auto & m : grassModes() )
    {
        if ( m.first == mode ) return QString::fromLatin1( m.second );
    }
    return QString();
}

/*!
  \brief the mode of the grass of a name in the options
  \return false if the name is unknown (mode is left as it was)
*/
inline
bool
grassModeFromName( const QString & name,
                   GrassMode * mode )
{
    for ( const auto & m : grassModes() )
    {
        if ( name == QLatin1String( m.second ) )
        {
            *mode = m.first;
            return true;
        }
    }
    return false;
}

/*!
  \enum ShadowMode
  \brief the sun's shadows of the players in the 3D view (docs/plan/2026-11-11_3d-view.md
  §4.3, 8-1): none, simple ones (a soft ellipse on the ground for each
  player, away from the sun: almost no cost), or detailed ones (a shadow
  map: the shapes of the players and the goal frames)
*/
enum class ShadowMode {
    Off,
    Simple,
    Detailed,
};

//! the shadows' modes, with their names in the options
inline
const std::array< std::pair< ShadowMode, const char * >, 3 > &
shadowModes()
{
    static const std::array< std::pair< ShadowMode, const char * >, 3 > list = { {
            { ShadowMode::Off, "off" },
            { ShadowMode::Simple, "simple" },
            { ShadowMode::Detailed, "detailed" },
        } };
    return list;
}

//! the name of a mode of the shadows in the options
inline
QString
shadowModeName( const ShadowMode mode )
{
    for ( const auto & m : shadowModes() )
    {
        if ( m.first == mode ) return QString::fromLatin1( m.second );
    }
    return QString();
}

/*!
  \brief the mode of the shadows of a name in the options
  \return false if the name is unknown (mode is left as it was)
*/
inline
bool
shadowModeFromName( const QString & name,
                    ShadowMode * mode )
{
    for ( const auto & m : shadowModes() )
    {
        if ( name == QLatin1String( m.second ) )
        {
            *mode = m.first;
            return true;
        }
    }
    return false;
}

//! a close-up camera (CameraMode::CloseupBall, CloseupPlayer)
inline
bool
isCloseup( const CameraMode mode )
{
    return mode == CameraMode::CloseupBall || mode == CameraMode::CloseupPlayer;
}

#endif
