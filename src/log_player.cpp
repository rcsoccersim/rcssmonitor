// -*-c++-*-

/*!
  \file log_player.cpp
  \brief log player class Header File.
*/

/*
 *Copyright:

 Copyright (C) The RoboCup Soccer Server Maintenance Group.
 Hidehisa AKIYAMA

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

#include <QApplication>
#include <QTimer>

#include "disp_holder.h"
#include "log_player.h"
#include "options.h"

#include <iostream>

/*-------------------------------------------------------------------*/
/*!

*/
LogPlayer::LogPlayer( DispHolder & disp_holder,
                      QObject * parent )
    : QObject( parent ),
      M_disp_holder( disp_holder ),
      M_timer( new QTimer( this ) ),
      M_live_mode( false )
{
    connect( M_timer, SIGNAL( timeout() ),
             this, SLOT( handleTimer() ) );

    M_timer->setInterval( Options::instance().timerInterval() );
}

/*-------------------------------------------------------------------*/
/*!

*/
LogPlayer::~LogPlayer()
{

}

/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayer::clear()
{
    if ( M_timer->isActive() )
    {
        M_timer->stop();
    }

    M_live_mode = false;

    M_timer->setInterval( Options::instance().timerInterval() );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayer::handleTimer()
{
    stepForwardImpl();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayer::stepBackwardImpl()
{
    if ( M_disp_holder.setIndexStepBack() )
    {
        emit indexUpdated( M_disp_holder.currentIndex(), M_disp_holder.dispCont().size() );
        //emit updated();
    }
    else
    {
        M_timer->stop();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayer::stepForwardImpl()
{
    if ( M_disp_holder.setIndexStepForward() )
    {
        emit indexUpdated( M_disp_holder.currentIndex(), M_disp_holder.dispCont().size() );
        //emit updated();
    }
    else
    {
        M_timer->stop();
    }

    if ( Options::instance().autoReconnectMode() )
    {

    }
    else if ( Options::instance().autoQuitMode() )
    {
        DispConstPtr disp = M_disp_holder.currentDisp();
        if ( disp
             && disp->pmode_ == rcss::rcg::PM_TimeOver )
        {
            emit quitRequested();
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayer::stepBackward()
{
    M_live_mode = false;
    M_timer->stop();

    stepBackwardImpl();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayer::stepForward()
{
    M_live_mode = false;
    M_timer->stop();

    stepForwardImpl();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayer::playOrStop( bool play )
{
    M_live_mode = false;

    if ( play )
    {
        playForward();
    }
    else
    {
        if ( M_timer->isActive() )
        {
            M_timer->stop();
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayer::stop()
{
    M_live_mode = false;
    M_timer->stop();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayer::playBackward()
{
    M_live_mode = false;

    if ( ! M_timer->isActive()
         || M_timer->interval() != Options::instance().timerInterval() )
    {
        M_timer->start( Options::instance().timerInterval() );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayer::playForward()
{
    M_live_mode = false;

    if ( ! M_timer->isActive()
         || M_timer->interval() != Options::instance().timerInterval() )
    {
        M_timer->start( Options::instance().timerInterval() );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayer::goToPreviousScore()
{
    M_live_mode = false;

    const std::size_t cur_idx = M_disp_holder.currentIndex();


    for ( std::vector< std::size_t >::const_reverse_iterator it = M_disp_holder.scoreChangedIndex().rbegin(), rend = M_disp_holder.scoreChangedIndex().rend();
          it != rend;
          ++it )
    {
        if ( *it < cur_idx )
        {
            std::size_t new_idx = ( *it < 50
                                    ? 0
                                    : *it - 50 );
            goToIndex( new_idx );
            return;
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayer::goToNextScore()
{
    M_live_mode = false;

    const std::size_t cur_idx = M_disp_holder.currentIndex();

    for ( std::vector< std::size_t >::const_iterator it = M_disp_holder.scoreChangedIndex().begin(), end = M_disp_holder.scoreChangedIndex().end();
          it != end;
          ++it )
    {
        if ( cur_idx < *it - 50 )
        {
            std::size_t new_idx = ( *it < 50
                                    ? 0
                                    : *it - 50 );
            goToIndex( new_idx );
            return;
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayer::goToFirst()
{
    if ( M_disp_holder.setIndexFirst() )
    {
        M_live_mode = false;
        M_timer->stop();

        emit indexUpdated( M_disp_holder.currentIndex(), M_disp_holder.dispCont().size() );
        //emit updated();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayer::goToLast()
{
    if ( M_disp_holder.setIndexLast() )
    {
        M_live_mode = false;
        M_timer->stop();

        emit indexUpdated( M_disp_holder.currentIndex(), M_disp_holder.dispCont().size() );
        //emit updated();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayer::decelerate()
{
    if ( M_timer->isActive() )
    {
        const int interval = std::min( 5000, M_timer->interval() * 2 );
        M_timer->start( interval );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayer::accelerate()
{
    if ( M_timer->isActive() )
    {
        const int interval = std::max( 5, M_timer->interval() / 2 );
        M_timer->start( interval );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayer::goToIndex( int index )
{
    if ( M_disp_holder.setIndex( index ) )
    {
        M_live_mode = false;
        //M_timer->stop();

        emit indexUpdated( M_disp_holder.currentIndex(), M_disp_holder.dispCont().size() );
        //emit updated();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayer::goToCycle( int cycle )
{
    if ( M_disp_holder.setCycle( cycle ) )
    {
        M_live_mode = false;
        //M_timer->stop();

        emit indexUpdated( M_disp_holder.currentIndex(), M_disp_holder.dispCont().size() );
        //emit updated();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayer::showLive()
{
    if ( M_disp_holder.setIndexLast() )
    {
        M_timer->stop();

        emit indexUpdated( M_disp_holder.currentIndex(), M_disp_holder.dispCont().size() );
        //emit updated();
    }

    if ( M_disp_holder.playmode() == rcss::rcg::PM_TimeOver )
    {
        if ( Options::instance().autoReconnectMode() )
        {

        }
        else if ( Options::instance().autoQuitMode() )
        {
            emit quitRequested();
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayer::setLiveMode()
{
    M_disp_holder.setIndexLast();
    M_live_mode = true;
    M_timer->stop();

    //emit updated();
}


/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayer::startTimer()
{
    if ( ! M_disp_holder.dispCont().empty()
         && ! M_timer->isActive() )
    {
        M_timer->start( Options::instance().timerInterval() );
    }
}
