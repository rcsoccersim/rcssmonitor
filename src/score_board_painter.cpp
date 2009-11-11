// -*-c++-*-

/*!
  \file score_board_painter.cpp
  \brief score board painter class Source File.
*/

/*
 *Copyright:

 Copyright (C) The RoboCup Soccer Server Maintenance Group.
 Hidehisa Akiyama

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
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

#include <QtGui>

#include "score_board_painter.h"

#include "disp_holder.h"
#include "options.h"

#include <iostream>

/*-------------------------------------------------------------------*/
/*!

*/
ScoreBoardPainter::ScoreBoardPainter( const DispHolder & disp_holder )
    : M_disp_holder( disp_holder )
{
    //M_font.setBold( true );
    //M_font.setStyleHint( QFont::System, QFont::PreferBitmap );
    //M_font.setFixedPitch( true );
}

/*-------------------------------------------------------------------*/
/*!

*/
ScoreBoardPainter::~ScoreBoardPainter()
{

}

/*-------------------------------------------------------------------*/
/*!

*/
void
ScoreBoardPainter::draw( QPainter & painter )
{
    static const std::string s_playmode_strings[] = PLAYMODE_STRINGS;

    const Options & opt = Options::instance();

    if ( ! opt.showScoreBoard() )
    {
        return;
    }

    DispConstPtr disp = M_disp_holder.currentDisp();

    if ( ! disp )
    {
        return;
    }

    const int current_time = disp->show_.time_;

    const rcss::rcg::TeamT & team_l = disp->team_[0];
    const rcss::rcg::TeamT & team_r = disp->team_[1];

    const rcss::rcg::PlayMode pmode = disp->pmode_;

    const std::vector< std::pair< int, rcss::rcg::PlayMode > > & pen_scores_l = M_disp_holder.penaltyScoresLeft();
    const std::vector< std::pair< int, rcss::rcg::PlayMode > > & pen_scores_r = M_disp_holder.penaltyScoresRight();

    bool show_pen_score = true;

    if ( pen_scores_l.empty()
         && pen_scores_r.empty() )
    {
        show_pen_score = false;
    }
    else if ( ( ! pen_scores_l.empty()
                && current_time < pen_scores_l.front().first )
              && ( ! pen_scores_r.empty()
                   && current_time < pen_scores_r.front().first )
              && pmode != rcss::rcg::PM_PenaltySetup_Left
              && pmode != rcss::rcg::PM_PenaltySetup_Right
              && pmode != rcss::rcg::PM_PenaltyReady_Left
              && pmode != rcss::rcg::PM_PenaltyReady_Right
              && pmode != rcss::rcg::PM_PenaltyTaken_Left
              && pmode != rcss::rcg::PM_PenaltyTaken_Right )
    {
        show_pen_score = false;
    }


    QString main_buf;

    if ( ! show_pen_score )
    {
        main_buf.sprintf( " %10s %d:%d %-10s %19s %6d    ",
                          ( team_l.name_.empty() || team_l.name_ == "null" )
                          ? ""
                          : team_l.name_.c_str(),
                          team_l.score_,
                          team_r.score_,
                          ( team_r.name_.empty() || team_r.name_ == "null" )
                          ? ""
                          : team_r.name_.c_str(),
                          s_playmode_strings[pmode].c_str(),
                          current_time );
    }
    else
    {
        std::string left_penalty; left_penalty.reserve( 10 );
        std::string right_penalty; right_penalty.reserve( 10 );

        for ( std::vector< std::pair< int, rcss::rcg::PlayMode > >::const_iterator it = pen_scores_l.begin();
              it != pen_scores_l.end();
              ++it )
        {
            if ( it->first > current_time ) break;

            if ( it->second == rcss::rcg::PM_PenaltyScore_Left
                 || it->second == rcss::rcg::PM_PenaltyScore_Right )
            {
                left_penalty += 'o';
            }
            else if ( it->second == rcss::rcg::PM_PenaltyMiss_Left
                      || it->second == rcss::rcg::PM_PenaltyMiss_Right )
            {
                left_penalty += 'x';
            }
        }

        for ( std::vector< std::pair< int, rcss::rcg::PlayMode > >::const_iterator it = pen_scores_r.begin();
              it != pen_scores_r.end();
              ++it )
        {
            if ( it->first > current_time ) break;

            if ( it->second == rcss::rcg::PM_PenaltyScore_Left
                 || it->second == rcss::rcg::PM_PenaltyScore_Right )
            {
                right_penalty += 'o';
            }
            else if ( it->second == rcss::rcg::PM_PenaltyMiss_Left
                      || it->second == rcss::rcg::PM_PenaltyMiss_Right )
            {
                right_penalty += 'x';
            }
        }

        main_buf.sprintf( " %10s %d:%d |%-5s:%-5s| %-10s %19s %6d",
                          ( team_l.name_.empty() || team_l.name_ == "null" )
                          ? ""
                          : team_l.name_.c_str(),
                          team_l.score_, team_r.score_,
                          left_penalty.c_str(),
                          right_penalty.c_str(),
                          ( team_r.name_.empty() || team_r.name_ == "null" )
                          ? ""
                          : team_r.name_.c_str(),
                          s_playmode_strings[pmode].c_str(),
                          current_time );
    }

    //painter.setFont( M_font );
    painter.setFont( opt.scoreBoardFont() );
    QRect bounding_rect = painter.fontMetrics().boundingRect( main_buf );

    QRect rect;
    rect.setLeft( 0 );
    rect.setTop( painter.window().bottom() - bounding_rect.height() + 1 );
    rect.setWidth( bounding_rect.width() );
    rect.setHeight( bounding_rect.height() );

    painter.fillRect( rect, opt.scoreBoardBrush() );

    painter.setPen( opt.scoreBoardPen() );
    painter.setBrush( Qt::NoBrush );
    painter.drawText( rect,
                      Qt::AlignVCenter,
                      main_buf );
}
