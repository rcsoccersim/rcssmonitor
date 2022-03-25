// -*-c++-*-

/*!
  \file team_graphic.h
  \brief team graphic data holder class Header File
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
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.	If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifndef RCSS_RCG_TEAM_GRAPHIC_H
#define RCSS_RCG_TEAM_GRAPHIC_H

#include <rcss/rcg/xpm_tile.h>

#include <memory>
#include <vector>
#include <map>
#include <string>

namespace rcss {
namespace rcg {

/*!
  \class TeamGraphic
  \brief team graphic data holder
*/
class TeamGraphic {
public:

    //! max pixel width of the team graphic
    static constexpr size_t MAX_WIDTH = 256;
    //! max pixel height of the team graphic
    static constexpr size_t MAX_HEIGHT = 64;
    //! max colors
    static constexpr size_t MAX_COLOR = 255;

    typedef std::pair< int, int > Index; //!<  xpm tile index
    typedef std::map< Index, XpmTile::Ptr > Map; //!< xpm tile map

private:
    size_t M_width; //!< total pixmap width
    size_t M_height; //!< total pixmap height

    //! 8x8 xpm tiles
    Map M_tiles;

public:

    /*!
      \brief initialize member variables with illegal values.
    */
    TeamGraphic();

    /*!
      \brief erase all data
    */
    void clear();

    /*!
      \brief geth the total width of this team graphic
      \return pixel width value
    */
    size_t width() const
    {
        return M_width;
    }

    /*!
      \brief geth the total height of this team graphic
      \return pixel height value
    */
    size_t height() const
    {
        return M_height;
    }

    /*!
      \brief get the tiled xpm data
      \return tiled xpm data map container
    */
    const Map & tiles() const
    {
        return M_tiles;
    }

    /*!
      \brief add a tile to the holder
      \param x index value
      \param y index value
      \param tile pointer to the instance
     */
    bool addXpmTile( const int x,
                     const int y,
                     XpmTile::Ptr tile );

    /*!
      \brief create tiled xpm from the raw xpm data
      \param xpm_data raw xpm string array
      \return true if successfully parsed
    */
    bool createFromRawXpm( const char * const * xpm_data );

    /*!
      \brief analyze team_graphic_? message from rcssserver & add new xpm tile
      \param server_msg raw server message
      \return true if successfully analyzed
    */
    bool createFromServerMessage( const char * server_msg );

    /*!
      \brief output all tiled xpm data
      \param os reference to the output stream
      \return reference to the output stream
    */
    std::ostream & print( std::ostream & os ) const;
};

}
}

#endif
