// -*-c++-*-

/*!
  \file log_file_name.h
  \brief the names of game log files: the date, the teams and the score.
  Header File.
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

#ifndef RCSSMONITOR_LOG_FILE_NAME_H
#define RCSSMONITOR_LOG_FILE_NAME_H

#include <QDateTime>
#include <QString>

/*!
  \class LogFileName
  \brief the name of a game log file read (docs/plan/2026-11-22-02_remote-logs.md
  4.2), as rcssserver names them: DATE-LEFT_SCORE-vs-RIGHT_SCORE.rcg.gz, the
  date of 12 digits (yyyyMMddhhmm) or 14 (to the second), and a score
  _GOALS or _GOALS_PENALTIES (a penalty shootout). A team's name may have
  "-" and "_" (e.g. FC-AIT_2, RoboTech_Coach_base_0).
*/
class LogFileName {
private:

    bool M_valid;
    QDateTime M_date_time;
    QString M_left_team;
    QString M_right_team;
    int M_left_score;
    int M_right_score;
    int M_left_penalties; //!< -1 without a penalty shootout
    int M_right_penalties;

public:

    //! a name not read
    LogFileName();

    /*!
      \brief read a file name (without its directory)
      \return the name read, or one not valid if it does not have the form
    */
    static
    LogFileName parse( const QString & file_name );

    //! whether the name had the form
    bool isValid() const { return M_valid; }

    const QDateTime & dateTime() const { return M_date_time; }
    const QString & leftTeam() const { return M_left_team; }
    const QString & rightTeam() const { return M_right_team; }
    int leftScore() const { return M_left_score; }
    int rightScore() const { return M_right_score; }
    //! the goals of the penalty shootout, or -1 without one
    int leftPenalties() const { return M_left_penalties; }
    int rightPenalties() const { return M_right_penalties; }

    //! the score to show: "2 - 1", or "0 (4) - 0 (2)" with a penalty shootout; empty if not valid
    QString score() const;
};

#endif
