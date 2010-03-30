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
      M_forward( true ),
      M_live_mode( false ),
      M_need_recovering( false )
{
    connect( M_timer, SIGNAL( timeout() ),
             this, SLOT( handleTimer() ) );

    M_timer->setInterval( Options::instance().timerInterval() );

    Options::instance().setBufferRecoverMode( true );
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

    M_forward = true;
    M_live_mode = false;
    M_need_recovering = false;
    M_timer->setInterval( Options::instance().timerInterval() );

    Options::instance().setBufferRecoverMode( true );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayer::handleTimer()
{
    const Options & opt = Options::instance();

    if ( ! opt.bufferingMode()
         || ! opt.bufferRecoverMode()
         || M_disp_holder.currentIndex() == DispHolder::INVALID_INDEX )
    {
        if ( M_forward )
        {
            stepForwardImpl();
        }
        else
        {
            stepBackImpl();
        }
    }

    adjustTimer();

    if ( opt.bufferingMode()
         && opt.bufferRecoverMode() )
    {
        emit recoverTimerHandled();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayer::stepBackImpl()
{
    if ( M_disp_holder.setIndexStepBack() )
    {
        emit updated();
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
        emit updated();
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
LogPlayer::stepBack()
{
    M_live_mode = false;
    M_forward = false;
    M_timer->stop();

    stepBackImpl();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayer::stepForward()
{
    M_live_mode = false;
    M_forward = true;
    M_timer->stop();

    stepForwardImpl();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayer::playOrStop()
{
    M_live_mode = false;

    if ( M_timer->isActive() )
    {
        M_timer->stop();
    }
    else if ( M_forward )
    {
        playForward();
    }
    else
    {
        playBack();
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
LogPlayer::playBack()
{
    M_live_mode = false;
    M_forward = false;

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
    M_forward = true;

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
LogPlayer::accelerateBack()
{
    int interval = 0;
    if ( M_forward
         || ! M_timer->isActive() )
    {
        interval = Options::instance().timerInterval() / 2;
    }
    else
    {
        interval = M_timer->interval() / 2;
        if ( interval < 10 ) interval = 10;
    }

    M_live_mode = false;
    M_forward = false;
    M_timer->start( interval );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayer::accelerateForward()
{
    int interval = 0;
    if ( ! M_forward
        || ! M_timer->isActive() )
    {
        interval = Options::instance().timerInterval() / 2;
    }
    else
    {
        interval = M_timer->interval() / 2;
        if ( interval < 10 ) interval = 10;
    }

    M_live_mode = false;
    M_forward = true;
    M_timer->start( interval );
}

/*-------------------------------------------------------------------*/
/*!

*/
// void
// LogPlayer::goToPrevScore()
// {
//     M_live_mode = false;

//     const DispHolder & holder = M_main_data.dispHolder();

//     const std::size_t cur_idx = M_main_data.index();
//     std::vector< std::size_t >::const_reverse_iterator rend = holder.scoreChangedIndex().rend();
//     for ( std::vector< std::size_t >::const_reverse_iterator it = holder.scoreChangedIndex().rbegin();
//           it != rend;
//           ++it )
//     {
//         if ( *it < cur_idx )
//         {
//             std::size_t new_idx = *it;
//             new_idx -= ( new_idx < 50 ? new_idx : 50 );
//             goToIndex( new_idx );
//             return;
//         }
//     }
// }

/*-------------------------------------------------------------------*/
/*!

*/
// void
// LogPlayer::goToNextScore()
// {
//     M_live_mode = false;

//     const DispHolder & holder = M_main_data.dispHolder();

//     const std::size_t cur_idx = M_main_data.index();
//     std::vector< std::size_t >::const_iterator end = holder.scoreChangedIndex().end();
//     for ( std::vector< std::size_t >::const_iterator it = holder.scoreChangedIndex().begin();
//           it != end;
//           ++it )
//     {
//         if ( 50 < *it && cur_idx < *it - 50 )
//         {
//             std::size_t new_idx = *it;
//             new_idx -= ( new_idx < 50 ? 0 : 50 );
//             goToIndex( new_idx );
//             return;
//         }
//     }
// }

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

        emit updated();
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

        emit updated();
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
        int interval = M_timer->interval() * 2;
        if ( 5000 < interval ) interval = 5000;
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
        int interval = M_timer->interval() / 2;
        if ( interval < 5 ) interval = 5;
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

        emit updated();
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

        emit updated();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayer::showLive()
{
    if ( M_timer->isActive() )
    {
        M_timer->stop();
    }

    if ( Options::instance().bufferingMode() )
    {
        if ( M_disp_holder.setIndexLast() )
        {
            emit updated();
        }
    }
    else
    {
        M_disp_holder.setIndexLast();
        emit updated();
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

/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayer::adjustTimer()
{
    const Options & opt = Options::instance();

    M_live_mode = false;

    const int buffer_size = M_disp_holder.dispCont().size();

    if ( buffer_size == 0 )
    {
        if ( M_timer->isActive() )
        {
            M_timer->stop();
        }
        return;
    }

    const rcss::rcg::ServerParamT & SP = M_disp_holder.serverParam();

    int interval = std::min( opt.timerInterval(), SP.simulator_step_ );

    size_t current = M_disp_holder.currentIndex();
    if ( current == DispHolder::INVALID_INDEX )
    {
        Options::instance().setBufferRecoverMode( true );

        if ( M_timer->interval() != interval
             || ! M_timer->isActive() )
        {
            M_timer->start( interval );
        }

        return;
    }

    const int max_cache_size = std::max( 1, opt.bufferSize() );
    const int current_cache = buffer_size - current - 1;

//     std::cerr << "index=" << current
//               << " buffer_size=" << buffer_size
//               << " max_cache_size=" << max_cache_size
//               << " current_cache=" << current_cache
//               << std::endl;

    if ( ! opt.monitorClientMode()
         || M_disp_holder.playmode() == rcss::rcg::PM_TimeOver )
    {
        Options::instance().setBufferRecoverMode( false );
    }
    else if ( current_cache <= 1 )
    {
        Options::instance().setBufferRecoverMode( true );
    }
    else if ( current_cache >= max_cache_size )
    {
        Options::instance().setBufferRecoverMode( false );
    }

    if ( ! opt.monitorClientMode()
         || M_disp_holder.playmode() == rcss::rcg::PM_TimeOver )
    {
        // default interval
    }
    else if ( opt.bufferRecoverMode() )
    {
        interval = std::max( opt.timerInterval(), SP.simulator_step_ );
    }
    else if ( current_cache >= max_cache_size )
    {
        M_need_recovering = false;
    }
    else if ( M_need_recovering )
    {
        interval = std::max( opt.timerInterval(), SP.simulator_step_ ) * 11 / 10;
    }
    else if ( current_cache <= max_cache_size / 2
              && M_disp_holder.playmode() != rcss::rcg::PM_TimeOver )
    {
        M_need_recovering = true;
        interval = std::max( opt.timerInterval(), SP.simulator_step_ ) * 11 / 10;
    }
    else if ( current_cache > max_cache_size / 2 )
    {
        interval = std::max( opt.timerInterval(), SP.simulator_step_ );
    }

#if 0
    if ( M_timer->interval() != interval )
    {
        std::cerr << "change interval " << interval << std::endl;
    }
#endif

//     std::cout << "disp_size=" << buffer_size
//               << " current_cache=" << current_cache
//               << " interval=" << interval
//               << std::endl;

    if ( M_timer->interval() != interval
         || ! M_timer->isActive() )
    {
        M_timer->start( interval );
    }
}
