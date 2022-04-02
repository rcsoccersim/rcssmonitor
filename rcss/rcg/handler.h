// -*-c++-*-

/*!
  \file handler.h
  \brief rcg data handler Header File.
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

#ifndef RCSS_RCG_HANDLER_H
#define RCSS_RCG_HANDLER_H

#include <rcss/rcg/types.h>

#include <string>
#include <memory>

namespace rcss {
namespace rcg {

class XpmTile;

/*!
  \class Handler
  \brief abstract rcg data handler class.

  This is an interface class, all member methods are virtual.
*/
class Handler {
private:

    //! RCG version number(1-3, default:0)
    int M_log_version;

protected:

    /*!
      \brief default constructor. log version and time are set to zero.
    */
    Handler();

public:
    /*!
      \brief virtual destructor
    */
    virtual
    ~Handler();

    /*!
      \brief records rcg version
      \param ver log version.
      \return always true.

      This method is virtual.
      You can override this.
    */
    virtual
    bool handleLogVersion( const int ver )
      {
          M_log_version = ver;
          return true;
      }

    /*!
      \brief returns rcg version number
      \return rcg version number

      This method is virtual.
      You can override this.
    */
    virtual
    int logVersion() const
      {
          return M_log_version;
      }

    /*!
      \brief (pure virtual) handle end of file
      \return result status
    */
    virtual
    bool handleEOF() = 0;

    /*!
      \brief (pure virtual) handle the start of show info v4
      \param time game time of handled show info
      \param show read data
      \return result status
     */
    virtual
    bool handleShow( const ShowInfoT & show ) = 0;

    /*!
      \brief handle playmode
      \param time game time of handled playmode info
      \param pm playmode id
      \return result status
     */
    virtual
    bool handlePlayMode( const int time,
                         const PlayMode pm ) = 0;

    /*!
      \brief handle team info
      \param time game time of handled team info
      \param team_l left team info
      \param team_r right team info
      \return result status
    */
    virtual
    bool handleTeam( const int time,
                     const TeamT & team_l,
                     const TeamT & team_r ) = 0;

    /*!
      \brief (pure virtual) handle msg info
      \param time game time of handled msg info
      \param board message board type
      \param msg new data
      \return result status
     */
    virtual
    bool handleMsg( const int time,
                    const int board,
                    const std::string & msg ) = 0;

    /*!
      \brief (pure virtual) handle draw clear
      \param time game time
      \return result status
     */
    virtual
    bool handleDrawClear( const int time ) = 0;

    /*!
      \brief (pure virtual) handle draw point
      \param time game time
      \param point point information
      \return result status
     */
    virtual
    bool handleDrawPoint( const int time,
                          const PointT & point ) = 0;

    /*!
      \brief (pure virtual) handle draw circle
      \param time game time
      \param circle circle information
      \return result status
     */
    virtual
    bool handleDrawCircle( const int time,
                           const CircleT & circle ) = 0;

    /*!
      \brief (pure virtual) handle draw circle
      \param time game time
      \param line line information
      \return result status
     */
    virtual
    bool handleDrawLine( const int time,
                         const LineT & line ) = 0;

    /*!
      \brief handle server_param message
      \param msg raw message string
      \return result status
    */
    virtual
    bool handleServerParam( const ServerParamT & sparam ) = 0;

    /*!
      \brief handle player_param message
      \param msg raw message string
      \return result status
    */
    virtual
    bool handlePlayerParam( const PlayerParamT & pparam ) = 0;

    /*!
      \brief handle player_type message
      \param msg raw message string
      \return result status
    */
    virtual
    bool handlePlayerType( const PlayerTypeT & ptype ) = 0;

    /*!
      \brief handle XPM tile of TeamGraphic
     */
    virtual
    bool handleTeamGraphic( const Side side,
                            const int x,
                            const int y,
                            std::shared_ptr< XpmTile > tile ) = 0;

};


} // end of namespace
} // end of namespace

#endif
