// -*-c++-*-

/*!
  \file xpm_tile.h
  \brief XPM tile for team_graphic Header File.
*/

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 3 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifndef RCSS_RCG_XPM_TILE_H
#define RCSS_RCG_XPM_TILE_H

#include <vector>
#include <string>
#include <memory>

namespace rcss {
namespace rcg {

/*!
  \class XPMTile
  \brief xpm tile for team_graphic. The size must be always 8x8 pixels
 */
class XpmTile {
public:
    static constexpr size_t WIDTH = 8; //!< fixed tile width
    static constexpr size_t HEIGHT = 8; //!< fixed tile height
    static constexpr size_t CPP = 1; //!< fixed char per pixel

    typedef std::shared_ptr< XpmTile > Ptr;

private:

    //! the number of colors
    size_t M_color_size;
    //! xpm information (width, height, # of colors, cpp)
    std::string M_header;
    //! color definitions
    std::vector< std::string > M_colors;
    //! pixel lines
    std::vector< std::string > M_pixel_lines;

public:

    /*!
      \brief default constructor
    */
    XpmTile()
        : M_color_size( 0 )
    {
        M_pixel_lines.reserve( HEIGHT );
    }

    /*!
      \brief check if the data is valid or not
      \return checked result
     */
    bool isValid() const;

    /*!
      \brief get the header line
     */
    const std::string & header() const
    {
        return M_header;
    }

    /*!
      \brief get the color data array
      \return color string container
    */
    const std::vector< std::string > & colors() const
    {
        return M_colors;
    }

    /*!
      \brief get pixel lines
      \return pixel line container
    */
    const std::vector< std::string > & pixelLines() const
    {
        return M_pixel_lines;
    }

    /*!
      \brief add new data block
      \return the result of consistency check
     */
    bool addData( const char * data );

    bool setHeader( const char * data );
    bool addColor( const char * data );
    bool addPixelLine( const char * data );

public:

    /*!
      \brief print xpm data array
      \param os output stream
      \param sep separator charactor
      \return output stream
     */
    std::ostream & print( std::ostream & os,
                          const char sep ) const;
};

} // end of namespace
} // end of namespace

#endif
