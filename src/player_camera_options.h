// -*-c++-*-

/*!
  \file player_camera_options.h
  \brief the styles of the selected player's camera of the 3D view. Header File.
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


#ifndef RCSSMONITOR_PLAYER_CAMERA_OPTIONS_H
#define RCSSMONITOR_PLAYER_CAMERA_OPTIONS_H

#include <QString>

#include <array>

/*!
  \brief the styles of the selected player's camera of the 3D view
  (rcssmonitor-web player-camera.ts, docs/PLAN_3D-VIEW.md §4.1, §5.7):
  where it stands, which direction it follows, how it turns, how wide it
  sees, and what it draws. Every combination can be chosen. Here, with no
  OpenGL, so that the options know them in a build without the 3D view too.
*/
namespace PlayerCameraStyle {

//! where the camera stands
enum class Position {
    Eye, //!< at the eyes
    Shoulder, //!< behind and above the player
};

//! the direction it follows
enum class Direction {
    Head, //!< the face: body + neck
    Body, //!< the body
    Middle, //!< halfway: body + neck / 2
};

//! how it turns
enum class Motion {
    Instant, //!< to the direction at once
    Smooth, //!< toward it with the time constant, never faster than the largest turn
};

//! how wide it sees (horizontally)
enum class Fov {
    View, //!< the view width at once
    Eased, //!< toward the view width with the time constant
    Fixed, //!< the fixed field of view
};

//! what it draws (Sight)
enum class Sight {
    All, //!< everything
    View, //!< what is inside the view cone
    Server, //!< what rcssserver's visual sensor tells the player
};

/*!
  \brief the name of a style in the options (as rcssmonitor-web's) and its
  label in the settings dialog (the Player Camera tab)
*/
template < typename E >
struct Info {
    E value;
    const char * name;
    const char * label;
};

inline const std::array< Info< Position >, 2 > & positions()
{
    static const std::array< Info< Position >, 2 > list = { {
            { Position::Eye, "eye", "Eyes" },
            { Position::Shoulder, "shoulder", "Over the Shoulder" },
        } };
    return list;
}

inline const std::array< Info< Direction >, 3 > & directions()
{
    static const std::array< Info< Direction >, 3 > list = { {
            { Direction::Head, "head", "Head" },
            { Direction::Body, "body", "Body" },
            { Direction::Middle, "middle", "Between" },
        } };
    return list;
}

inline const std::array< Info< Motion >, 2 > & motions()
{
    static const std::array< Info< Motion >, 2 > list = { {
            { Motion::Instant, "instant", "Instant" },
            { Motion::Smooth, "smooth", "Smooth" },
        } };
    return list;
}

inline const std::array< Info< Fov >, 3 > & fovs()
{
    static const std::array< Info< Fov >, 3 > list = { {
            { Fov::View, "view", "View Width" },
            { Fov::Eased, "eased", "Eased" },
            { Fov::Fixed, "fixed", "Fixed" },
        } };
    return list;
}

inline const std::array< Info< Sight >, 3 > & sights()
{
    static const std::array< Info< Sight >, 3 > list = { {
            { Sight::All, "all", "Everything" },
            { Sight::View, "view", "View Cone" },
            { Sight::Server, "server", "Server Rules" },
        } };
    return list;
}

//! the name of a value in the options
template < typename E, std::size_t N >
QString
nameOf( const std::array< Info< E >, N > & list,
        const E value )
{
    for ( const Info< E > & i : list )
    {
        if ( i.value == value ) return QString::fromLatin1( i.name );
    }
    return QString();
}

/*!
  \brief the value of a name in the options
  \return false if the name is unknown (value is left as it was)
*/
template < typename E, std::size_t N >
bool
fromName( const std::array< Info< E >, N > & list,
          const QString & name,
          E * value )
{
    for ( const Info< E > & i : list )
    {
        if ( name == QLatin1String( i.name ) )
        {
            *value = i.value;
            return true;
        }
    }
    return false;
}

// the numbers and their ranges
constexpr double TIME_CONSTANT = 0.5; //!< of Smooth and Eased (s)
constexpr double MIN_TIME_CONSTANT = 0.0;
constexpr double MAX_TIME_CONSTANT = 5.0;
constexpr double MAX_TURN = 90.0; //!< of Smooth (degree / s)
constexpr double MIN_MAX_TURN = 1.0;
constexpr double MAX_MAX_TURN = 3600.0;
constexpr double FIXED_FOV = 120.0; //!< the horizontal field of view of Fixed (degree)
constexpr double MIN_FIXED_FOV = 20.0;
constexpr double MAX_FIXED_FOV = 120.0;

}

/*!
  \struct PlayerCameraOptions
  \brief the styles of the selected player's camera, with the defaults
  that the user chose for the web version by watching games: over the
  shoulder, turning smoothly toward the face, 120 degrees fixed, and
  drawing what the server tells the player
*/
struct PlayerCameraOptions {
    PlayerCameraStyle::Position position = PlayerCameraStyle::Position::Shoulder;
    PlayerCameraStyle::Direction direction = PlayerCameraStyle::Direction::Head;
    PlayerCameraStyle::Motion motion = PlayerCameraStyle::Motion::Smooth;
    PlayerCameraStyle::Fov fov = PlayerCameraStyle::Fov::Fixed;
    PlayerCameraStyle::Sight sight = PlayerCameraStyle::Sight::Server;
    double time_constant = PlayerCameraStyle::TIME_CONSTANT; //!< s, of Smooth and Eased
    double max_turn = PlayerCameraStyle::MAX_TURN; //!< degree / s, of Smooth
    double fixed_fov = PlayerCameraStyle::FIXED_FOV; //!< degree, of Fixed

    bool operator==( const PlayerCameraOptions & o ) const
      {
          return position == o.position && direction == o.direction && motion == o.motion
              && fov == o.fov && sight == o.sight && time_constant == o.time_constant
              && max_turn == o.max_turn && fixed_fov == o.fixed_fov;
      }
    bool operator!=( const PlayerCameraOptions & o ) const { return ! ( *this == o ); }
};

#endif
