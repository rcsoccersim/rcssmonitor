// -*-c++-*-

/*!
  \file log_filter.h
  \brief the filter of the list of game logs: the teams, the result and
  the dates. Header File.
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

#ifndef RCSSMONITOR_LOG_FILTER_H
#define RCSSMONITOR_LOG_FILTER_H

#include <QDate>
#include <QString>

class LogFileName;

/*!
  \class LogFilter
  \brief the conditions of the games shown in File > Browse Game Logs
  (docs/plan/2026-11-22-02_remote-logs.md 4.4, D14): a team on a side
  (either, left or right), its opponent, its result, and a range of dates.
  The names match a part of the teams' names, in any case. An empty
  condition is any.

  The result is seen from the team of team() (any team on its side if
  empty): by the goals, and in a tie with a penalty shootout by its goals.
*/
class LogFilter {
public:

    //! the side of the team of team()
    enum Side {
        EITHER_SIDE = 0,
        LEFT_SIDE,
        RIGHT_SIDE,
    };

    //! the result of the team of team()
    enum Result {
        ANY_RESULT = 0,
        WIN,
        DRAW,
        LOSS,
    };

private:

    QString M_team;
    Side M_side;
    QString M_opponent;
    Result M_result;
    QDate M_from; //!< the first day (invalid: no limit)
    QDate M_to; //!< the last day (invalid: no limit)

public:

    //! no condition
    LogFilter();

    const QString & team() const { return M_team; }
    Side side() const { return M_side; }
    const QString & opponent() const { return M_opponent; }
    Result result() const { return M_result; }
    const QDate & from() const { return M_from; }
    const QDate & to() const { return M_to; }

    //! the text is trimmed
    void setTeam( const QString & team );
    void setSide( const Side side ) { M_side = side; }
    void setOpponent( const QString & opponent );
    void setResult( const Result result ) { M_result = result; }
    void setFrom( const QDate & date ) { M_from = date; }
    void setTo( const QDate & date ) { M_to = date; }

    //! no condition: all the logs match
    bool isEmpty() const;

    /*!
      \brief whether a game log matches
      \param name its name read (LogFileName::parse())
      \param file_name its file name: if the name could not be read, it
      matches when it has the text of team() and no other condition is
      given
    */
    bool matches( const LogFileName & name,
                  const QString & file_name ) const;

    bool operator==( const LogFilter & other ) const;
    bool operator!=( const LogFilter & other ) const { return ! ( *this == other ); }

    /*!
      \brief the result of a game seen from a team
      \param goals, penalties the team's (penalties -1 without a shootout)
      \param other_goals, other_penalties the opponent's
    */
    static
    Result resultOf( const int goals,
                     const int penalties,
                     const int other_goals,
                     const int other_penalties );
};

#endif
