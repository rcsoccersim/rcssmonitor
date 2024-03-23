// -*-c++-*-

/*!
  \file team_graphic.h
  \brief team graphic data class Header File
*/

/*
 *Copyright:

 Copyright (C) Hidehisa Akiyama

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

#ifndef TEAM_GRAPHIC_H
#define TEAM_GRAPHIC_H

#include <memory>
#include <vector>
#include <map>
#include <string>

/*!
  \class TeamGraphic
  \brief team graphic data management class
*/
class TeamGraphic {
public:

    //! max pixel width of the team graphic
    static constexpr int MAX_WIDTH = 256;
    //! max pixel height of the team graphic
    static constexpr int MAX_HEIGHT = 64;
    //! tile size unit
    static constexpr int TILE_SIZE = 8;
    //! max colors
    static constexpr int MAX_COLOR = 256;
    //! characters per pixel
    static constexpr int CPP = 1;


    /*!
      \class XpmTile
      \brief 8x8 pixels xpm data contained by the TeamGraphic
    */
    class XpmTile {
    public:
        using Ptr = std::shared_ptr< XpmTile >; //!< XpmTile pointer

    private:
        const int M_width; //!< width of this tile
        const int M_height; //!< height of this tile
        const int M_cpp; //!< character per pixel of this tile

        //! shared color strings
        std::vector< std::shared_ptr< std::string > > M_colors;
        //! pixel lines
        std::vector< std::string > M_pixel_lines;

        // not used
        XpmTile() = delete;
    public:

        /*!
          \brief initialize xpm header information
          \param width xpm width
          \param height xpm height
          \param cpp characters per pixel
        */
        XpmTile( const int width,
                 const int height,
                 const int cpp );

        /*!
          \brief get the xpm width
          \return pixel width value
        */
        int width() const
          {
              return M_width;
          }

        /*!
          \brief get the xpm height
          \return pixel height value
        */
        int height() const
          {
              return M_width;
          }

        /*!
          \brief get the value of characters per pixel.
          \return the value of characters per pixel.
        */
        int cpp() const
          {
              return M_cpp;
          }

        /*!
          \brief get used color strings
          \return color string container
        */
        const std::vector< std::shared_ptr< std::string > > & colors() const
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
          \brief add xpm color data string
          \param color pointer to the color data string
        */
        void addColor( std::shared_ptr< std::string > color )
          {
              M_colors.push_back( color );
          }

        /*!
          \brief add pixel line data
          \param line pixel line string
        */
        void addPixelLine( const std::string & line )
          {
              M_pixel_lines.push_back( line );
          }

        /*!
          \brief output xpm lines
          \param os refrence to the output stream
          \param separator separator character
          \return refrence to the output stream
        */
        std::ostream & print( std::ostream & os,
                              const char separator ) const;
    };

    using Index = std::pair< int, int >; //!<  xpm tile index
    using Map = std::map< Index, const XpmTile::Ptr >; //!< xpm tile map

private:
    int M_width; //!< total pixmap width
    int M_height; //!< total pixmap height
    int M_cpp; //!< char per pixel

    //! color data strings
    std::vector< std::shared_ptr< std::string > > M_colors;

    //! xpm tiles with index
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
      \brief check if all xpm tiles have been completed or not
      \return checked result
    */
    bool isValid() const;

    /*!
      \brief geth the total width of this team graphic
      \return pixel width value
    */
    int width() const
      {
          return M_width;
      }

    /*!
      \brief geth the total height of this team graphic
      \return pixel height value
    */
    int height() const
      {
          return M_height;
      }

    /*!
      \brief get the xpm color data strings
      \return color strings container
    */
    const std::vector< std::shared_ptr< std::string > > & colors() const
      {
          return M_colors;
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
      \brief read xpm data from the input file
      \param file_path input file path
      \return parsing result
    */
    bool readXpmFile( const std::string & file_path );

    /*!
      \brief create tiled xpm from the raw xpm data
      \param xpm_data raw xpm string array
      \return true if successfully parsed
    */
    bool fromRawXpm( const char * const * xpm_data );

    /*!
      \brief create tiled xpm from the raw xpm data
      \param x xpm tile index
      \param y xpm tile index
      \param xpm_tile raw xpm string array
      \return true if successfully parsed
    */
    bool addXpmTile( const int x,
                     const int y,
                     const std::vector< std::string > & xpm_tile );

private:

    /*!
      \brief find string from the color string pool
      \param str searched string
      \return string pointer. if not found null pointer is returned.
    */
    std::shared_ptr< std::string > findColor( const std::string & str );

};

#endif
