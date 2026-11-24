// -*-c++-*-

/*!
  \file log_filter.cpp
  \brief the filter of the list of game logs: the teams, the result and
  the dates. Source File.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "log_filter.h"

#include "log_file_name.h"

/*-------------------------------------------------------------------*/
LogFilter::LogFilter()
    : M_side( EITHER_SIDE ),
      M_result( ANY_RESULT )
{

}

/*-------------------------------------------------------------------*/
void
LogFilter::setTeam( const QString & team )
{
    M_team = team.trimmed();
}

/*-------------------------------------------------------------------*/
void
LogFilter::setOpponent( const QString & opponent )
{
    M_opponent = opponent.trimmed();
}

/*-------------------------------------------------------------------*/
bool
LogFilter::isEmpty() const
{
    return M_team.isEmpty()
        && M_side == EITHER_SIDE
        && M_opponent.isEmpty()
        && M_result == ANY_RESULT
        && ! M_from.isValid()
        && ! M_to.isValid();
}

/*-------------------------------------------------------------------*/
LogFilter::Result
LogFilter::resultOf( const int goals,
                     const int penalties,
                     const int other_goals,
                     const int other_penalties )
{
    if ( goals != other_goals )
    {
        return goals > other_goals ? WIN : LOSS;
    }
    // a tie: the penalty shootout, if any (D14)
    if ( penalties >= 0
         && other_penalties >= 0
         && penalties != other_penalties )
    {
        return penalties > other_penalties ? WIN : LOSS;
    }
    return DRAW;
}

/*-------------------------------------------------------------------*/
bool
LogFilter::matches( const LogFileName & name,
                    const QString & file_name ) const
{
    if ( ! name.isValid() )
    {
        // no teams, no date: only by the text of the team in its name
        return M_side == EITHER_SIDE
            && M_opponent.isEmpty()
            && M_result == ANY_RESULT
            && ! M_from.isValid()
            && ! M_to.isValid()
            && file_name.contains( M_team, Qt::CaseInsensitive );
    }

    const QDate date = name.dateTime().date();
    if ( ( M_from.isValid() && date < M_from )
         || ( M_to.isValid() && date > M_to ) )
    {
        return false;
    }

    // the team on the left, or on the right
    const auto matches_from = [this, &name]( const bool left ) {
        const QString & team = left ? name.leftTeam() : name.rightTeam();
        const QString & other = left ? name.rightTeam() : name.leftTeam();
        if ( ! team.contains( M_team, Qt::CaseInsensitive )
             || ! other.contains( M_opponent, Qt::CaseInsensitive ) )
        {
            return false;
        }
        if ( M_result == ANY_RESULT )
        {
            return true;
        }
        const Result result = left
            ? resultOf( name.leftScore(), name.leftPenalties(), name.rightScore(), name.rightPenalties() )
            : resultOf( name.rightScore(), name.rightPenalties(), name.leftScore(), name.leftPenalties() );
        return result == M_result;
    };

    return ( M_side != RIGHT_SIDE && matches_from( true ) )
        || ( M_side != LEFT_SIDE && matches_from( false ) );
}

/*-------------------------------------------------------------------*/
bool
LogFilter::operator==( const LogFilter & other ) const
{
    return M_team == other.M_team
        && M_side == other.M_side
        && M_opponent == other.M_opponent
        && M_result == other.M_result
        && M_from == other.M_from
        && M_to == other.M_to;
}
