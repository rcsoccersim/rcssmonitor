// -*-c++-*-

/*!
  \file handler.h
  \brief Class for handring rcg data
*/

/*
 *Copyright:

 Copyright (C) The RoboCup Soccer Server Maintenance Group.
 Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

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

#ifndef RCSSLOGPLAYER_HANDLER_H
#define RCSSLOGPLAYER_HANDLER_H

#include <rcsslogplayer/types.h>

#include <string>

namespace rcss {
namespace rcg {

/*!
  class Handler
  \brief abstract rcg data handler
 */
class Handler {
protected:
    Handler()
      { }

public:

    virtual
    ~Handler()
      { }

private:
    friend class Parser;

    void handleLogVersion( int ver )
      {
          doHandleLogVersion( ver );
      }

    int getLogVersion() const
      {
          return doGetLogVersion();
      }

    void handleShowInfo( const ShowInfoT & info )
      {
          doHandleShowInfo( info );
      }

    void handleMsgInfo( const int time,
                        const int board,
                        const std::string & msg )
      {
          doHandleMsgInfo( time, board, msg );
      }

    void handlePlayMode( const int time,
                         const PlayMode playmode )
      {
          doHandlePlayMode( time, playmode );
      }

    void handleTeamInfo( const int time,
                         const TeamT & team_l,
                         const TeamT & team_r )
      {
          doHandleTeamInfo( time, team_l, team_r );
      }

    void handleDrawClear( const int time )
      {
          doHandleDrawClear( time );
      }

    void handleDrawPointInfo( const int time,
                              const PointInfoT & p )
      {
          doHandleDrawPointInfo( time, p );
      }

    void handleDrawCircleInfo( const int time,
                               const CircleInfoT & c )
      {
          doHandleDrawCircleInfo( time, c );
      }

    void handleDrawLineInfo( const int time,
                             const LineInfoT & l )
      {
          doHandleDrawLineInfo( time, l );
      }

    void handleServerParam( const ServerParamT & param )
      {
          doHandleServerParam( param );
      }

    void handlePlayerParam( const PlayerParamT & param )
      {
          doHandlePlayerParam( param );
      }

    void handlePlayerType( const PlayerTypeT & type )
      {
          doHandlePlayerType( type );
      }

    void handleEOF()
      {
          doHandleEOF();
      }

protected:
    virtual
    void doHandleLogVersion( int ver ) = 0;

    virtual
    int doGetLogVersion() const = 0;

    virtual
    void doHandleShowInfo( const ShowInfoT & ) = 0;

    virtual
    void doHandleMsgInfo( const int,
                          const int,
                          const std::string & ) = 0;

    virtual
    void doHandlePlayMode( const int,
                           const PlayMode ) = 0;

    virtual
    void doHandleTeamInfo( const int,
                           const TeamT &,
                           const TeamT & ) = 0;

    virtual
    void doHandleDrawClear( const int ) = 0;

    virtual
    void doHandleDrawPointInfo( const int,
                                const PointInfoT & ) = 0;

    virtual
    void doHandleDrawCircleInfo( const int,
                                 const CircleInfoT & ) = 0;

    virtual
    void doHandleDrawLineInfo( const int,
                               const LineInfoT & ) = 0;

    virtual
    void doHandleServerParam( const ServerParamT & ) = 0;

    virtual
    void doHandlePlayerParam( const PlayerParamT & ) = 0;

    virtual
    void doHandlePlayerType( const PlayerTypeT & ) = 0;

    virtual
    void doHandleEOF() = 0;

};

}
}

#endif
