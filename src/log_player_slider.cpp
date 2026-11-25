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

#include <QtWidgets>


#include "log_player_slider.h"

#include "ui_palette.h"

#include <algorithm>
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
    // as wide as the tool bar leaves (it takes the rest), at least this
    this->setMinimumWidth( 200 );
    this->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
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
void
LogPlayerSlider::setBands( const std::vector< Band > & bands )
{
    M_bands = bands;
    this->update();
}

/*-------------------------------------------------------------------*/
void
LogPlayerSlider::setGoals( const std::vector< std::size_t > & goals )
{
    M_goals = goals;
    this->update();
}

/*-------------------------------------------------------------------*/
int
LogPlayerSlider::positionOf( const std::size_t index ) const
{
    QStyleOptionSlider opt;
    initStyleOption( &opt );
    const QRect groove = style()->subControlRect( QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this );
    const QRect handle = style()->subControlRect( QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this );
    const int value = static_cast< int >( std::min< std::size_t >( index, std::max( 0, this->maximum() ) ) );
    // as the style places the handle: its middle at the value
    if ( this->orientation() == Qt::Horizontal )
    {
        return groove.x() + handle.width() / 2
            + QStyle::sliderPositionFromValue( this->minimum(), this->maximum(), value,
                                               groove.width() - handle.width(), opt.upsideDown );
    }
    return groove.y() + handle.height() / 2
        + QStyle::sliderPositionFromValue( this->minimum(), this->maximum(), value,
                                           groove.height() - handle.height(), opt.upsideDown );
}

/*-------------------------------------------------------------------*/
QColor
LogPlayerSlider::bandColor( const bool current ) const
{
    QColor color = this->palette().color( QPalette::Highlight );
    color.setAlpha( current ? 200 : 100 );
    return color;
}

/*-------------------------------------------------------------------*/
QColor
LogPlayerSlider::goalColor() const
{
    // an orange legible on the light window and on the dark one
    return UIPalette::isDark( this->palette() )
        ? QColor( 255, 183, 77 )
        : QColor( 211, 84, 0 );
}

/*-------------------------------------------------------------------*/
/*!
  The groove, the bands and the ticks of the goals on it, then the handle
  over them.
*/
void
LogPlayerSlider::paintEvent( QPaintEvent * event )
{
    if ( M_bands.empty() && M_goals.empty() )
    {
        QSlider::paintEvent( event );
        return;
    }

    QPainter painter( this );
    QStyleOptionSlider opt;
    initStyleOption( &opt );

    // as QSlider::paintEvent() draws, the handle apart
    opt.subControls = QStyle::SC_SliderGroove;
    if ( this->tickPosition() != QSlider::NoTicks )
    {
        opt.subControls |= QStyle::SC_SliderTickmarks;
    }
    style()->drawComplexControl( QStyle::CC_Slider, &opt, &painter, this );

    const QRect groove = style()->subControlRect( QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this );
    const bool horizontal = ( this->orientation() == Qt::Horizontal );
    const QPoint center = groove.center();

    // the bands across the middle of the groove, the current one last
    for ( const bool current : { false, true } )
    {
        for ( const Band & band : M_bands )
        {
            if ( band.current != current || band.first > band.last ) continue;
            const int a = positionOf( band.first );
            const int b = positionOf( band.last );
            const int lo = std::min( a, b ) - 1;
            const int len = std::max( 3, std::abs( b - a ) + 2 );
            const QRect rect = ( horizontal
                                 ? QRect( lo, center.y() - 3, len, 7 )
                                 : QRect( center.x() - 3, lo, 7, len ) );
            painter.fillRect( rect, bandColor( current ) );
        }
    }

    // the goals across the groove
    painter.setPen( QPen( goalColor(), 2 ) );
    for ( const std::size_t goal : M_goals )
    {
        const int p = positionOf( goal );
        if ( horizontal )
        {
            painter.drawLine( p, groove.top() - 1, p, groove.bottom() + 1 );
        }
        else
        {
            painter.drawLine( groove.left() - 1, p, groove.right() + 1, p );
        }
    }

    opt.subControls = QStyle::SC_SliderHandle;
    style()->drawComplexControl( QStyle::CC_Slider, &opt, &painter, this );
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
