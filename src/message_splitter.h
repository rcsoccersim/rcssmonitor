// -*-c++-*-

/*!
  \file message_splitter.h
  \brief splitter of the NUL terminated messages Header File.
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

#ifndef RCSSMONITOR_MESSAGE_SPLITTER_H
#define RCSSMONITOR_MESSAGE_SPLITTER_H

#include <QByteArray>

#include <deque>

/*!
  \class MessageSplitter
  \brief splits a byte stream into the messages terminated by '\0'.

  Empty messages are discarded. The terminating '\0' is not included
  in the stored messages.
*/
class MessageSplitter {
private:

    const int M_max_size; //!< the maximum length of an incomplete message
    QByteArray M_partial; //!< incomplete message
    std::deque< QByteArray > M_messages; //!< complete messages

public:

    /*!
      \brief constructor
      \param max_size the maximum length of an incomplete message
     */
    explicit
    MessageSplitter( const int max_size );

    /*!
      \brief append received bytes.
      \param data received bytes
      \return false if the incomplete message exceeds the maximum length.
     */
    bool append( const QByteArray & data );

    /*!
      \brief get one complete message.
      \param msg variable to store the message
      \return false if no complete message is available.
     */
    bool pop( QByteArray & msg );

    /*!
      \brief check if any complete message is available.
     */
    bool hasMessage() const
      {
          return ! M_messages.empty();
      }

    /*!
      \brief get the length of the incomplete message.
     */
    int partialSize() const
      {
          return M_partial.size();
      }

    /*!
      \brief discard all data.
     */
    void clear();
};

#endif
