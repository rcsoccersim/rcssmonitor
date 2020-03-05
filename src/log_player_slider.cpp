// -*-c++-*-

/*!
  \file log_slider_tool_bar.cpp
  \brief log player slider tool bar class Header File.
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

#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif


#include "log_player_slider.h"

#include <iostream>


/*-------------------------------------------------------------------*/
/*!

*/
LogPlayerSlider::LogPlayerSlider( QWidget * parent )
    : QSlider( Qt::Horizontal, parent )
{
    this->setStatusTip( tr( "Cycle Slider." ) );
    this->setToolTip( tr( "Cycle Slider" ) );

    this->setInvertedControls( true );

    this->setRange( 0, 0 );
    this->setValue( 1 );
    this->setTickInterval( 1 );
    //M_cycle_slider->setMaximumSize( 640, 640 );
    this->setMinimumWidth( 600 );
}


/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayerSlider::mousePressEvent( QMouseEvent * event )
{
    if ( event->button() == Qt::LeftButton )
    {
        double rate = 0;
        if ( this->orientation() == Qt::Horizontal )
        {
            rate = static_cast< double >( event->pos().x() )
                / static_cast< double >( this->width() );

        }
        else
        {
            rate = static_cast< double >( this->height() - event->pos().y() )
                / static_cast< double >( this->height() );
        }

        int val = this->minimum()
            + static_cast< int >( rint( ( this->maximum() - this->minimum() ) * rate ) );

        this->setValue( val );
    }

    QSlider::mousePressEvent( event );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayerSlider::changeOrientation( Qt::Orientation orientation )
{
    this->setOrientation( orientation );

   if ( orientation == Qt::Vertical )
    {
        this->setInvertedAppearance( true );
        this->setMinimumSize( 16, 100 );
        this->setMaximumSize( 16, 640 );
    }
    else
    {
        this->setInvertedAppearance( false );
        this->setMinimumSize( 200, 16 );
        this->setMaximumSize( 640, 16 );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
LogPlayerSlider::updateIndex( size_t current,
                              size_t maximum )
{
    // std::cerr << "Slider updateIndex " << current << " / " << maximum
    //           << " / slider_pos " << this->sliderPosition() << std::endl;

    if ( this->maximum() != (int)maximum - 1 )
    {
        this->setRange( 0, std::max( 1, (int)maximum - 1 ) );
    }

    if ( current != size_t( -1 )
         && this->sliderPosition() != (int)current )
    {
        this->setSliderPosition( current );
    }
}
