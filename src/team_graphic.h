// -*-c++-*-

/*!
  \file team_graphic.h
  \brief team graphic data class Header File
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

#ifndef RCSSMONITOR_TEAM_GRAPHIC_H
#define RCSSMONITOR_TEAM_GRAPHIC_H

#include <boost/shared_ptr.hpp>

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
    static const int MAX_WIDTH;
    //! max pixel height of the team graphic
    static const int MAX_HEIGHT;
    //! tile size unit
    static const int TILE_SIZE;
    //! max colors
    static const int MAX_COLOR;


    /*!
      \class XpmTile
      \brief 8x8 pixels xpm data contained by the TeamGraphic
    */
    class XpmTile {
    private:
        const int M_width; //!< width of this tile
        const int M_height; //!< height of this tile
        const int M_cpp; //!< character per pixel of this tile

        //! shared color strings
        std::vector< boost::shared_ptr< std::string > > M_colors;
        //! pixel lines
        std::vector< std::string > M_pixel_lines;

        // not used
        XpmTile();
    public:

        /*!
          \brief initialize xpm header information
          \param width xpm width
          \param height xpm height
          \param n_color number of
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
        const
        std::vector< boost::shared_ptr< std::string > > & colors() const
          {
              return M_colors;
          }

        /*!
          \brief get pixel lines
          \return pixel line container
        */
        const
        std::vector< std::string > & pixelLines() const
          {
              return M_pixel_lines;
          }

        /*!
          \brief add xpm color data string
          \param line pointer to the color data string
        */
        void addColor( boost::shared_ptr< std::string > color )
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
          \return refrence to the output stream
        */
        std::ostream & print( std::ostream & os ) const;
    };


    typedef boost::shared_ptr< XpmTile > Ptr; //!< XpmTile pointer
    typedef std::pair< int, int > Index; //!<  xpm tile index
    typedef std::map< Index, Ptr > Map; //!< xpm tile map

private:
    int M_width; //!< total pixmap width
    int M_height; //!< total pixmap height
    int M_cpp; //!< char per pixel

    //! color data strings
    std::vector< boost::shared_ptr< std::string > > M_colors;

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
    const
    std::vector< boost::shared_ptr< std::string > > & colors() const
      {
          return M_colors;
      }

    /*!
      \brief get the tiled xpm data
      \return tiled xpm data map container
     */
    const
    Map & tiles() const
      {
          return M_tiles;
      }

    /*!
      \brief create tiled xpm from the raw xpm data
      \param xpm_data raw xpm string array
      \return true if successfully parsed
     */
    bool createXpmTiles( const char * const * xpm_data );

    /*!
      \brief analyze team_graphic_? message from rcssserver & add new xpm tile
      \param server_msg raw server message
      \return true if successfully analyzed
     */
    bool parse( const char * server_msg );

private:

    /*!
      \brief find string from the color string pool
      \param str searched string
      \return string pointer. if not found null pointer is returned.
     */
    boost::shared_ptr< std::string > findColor( const std::string & str );

public:

    /*!
      \brief output all tiled xpm data
      \param os reference to the output stream
      \return reference to the output stream
     */
    std::ostream & print( std::ostream & os ) const;
};

#endif
