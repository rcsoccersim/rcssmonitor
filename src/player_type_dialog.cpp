// -*-c++-*-

/*!
  \file player_type_dialog.cpp
  \brief player type list ialog class Source File.
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

#include <QtGui>

#include "player_type_dialog.h"

#include "disp_holder.h"

#include <rcsslogplayer/types.h>

#include <iostream>
#include <cstdio>
#include <cmath>

namespace {
const int COL_SIZE = 15; // the number of param string
}

class PlayerTypeSortProxy
    : public QSortFilterProxyModel {
public:
    PlayerTypeSortProxy( QObject * parent )
        : QSortFilterProxyModel( parent )
      { }

protected:

    virtual
    bool lessThan( const QModelIndex & left,
                   const QModelIndex & right ) const
      {
          const QAbstractItemModel * src = sourceModel();

          double lhs = 0.0;
          double rhs = 0.0;
          std::sscanf( src->data( left ).toString().toAscii(), " %lf ", &lhs );
          std::sscanf( src->data( right ).toString().toAscii(), " %lf ", &rhs );

          return lhs < rhs;
      }
};

/*-------------------------------------------------------------------*/
/*!

*/
PlayerTypeDialog::PlayerTypeDialog( const DispHolder & disp_holder,
                                    QWidget * parent )

    : QDialog( parent, Qt::MSWindowsFixedSizeDialogHint )
    , M_disp_holder( disp_holder )
{
    this->setWindowTitle( tr( "Player Type List" ) );

    //QFont font = this->font();
    //font.setPointSize( 8 );
    //this->setFont( font );

    createTable();
}

/*-------------------------------------------------------------------*/
/*!

*/
PlayerTypeDialog::~PlayerTypeDialog()
{
    //std::cerr << "delete PlayerTypeDialog" << std::endl;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
PlayerTypeDialog::createTable()
{
    QVBoxLayout * layout = new QVBoxLayout();
    layout->setMargin( 4 );

    M_item_view = new QTableView();
    //M_item_view->setRootIsDecorated( false );
    //M_item_view->setGridStyle( Qt::NoPen );
    M_item_view->verticalHeader()->hide();
    M_item_view->setShowGrid( false );
    M_item_view->setAlternatingRowColors( true );
#if QT_VERSION >= 0x040200
    M_item_view->setSortingEnabled( true );
#endif

    M_item_view->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    M_item_view->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

    M_item_view->horizontalHeader()->setResizeMode( QHeaderView::ResizeToContents );

    QFont font = M_item_view->font();
    font.setPointSize( 9 );
    M_item_view->setFont( font );


    createModel();

    QSortFilterProxyModel * proxy = new PlayerTypeSortProxy( this );
    proxy->setSourceModel( M_model );
    M_item_view->setModel( proxy );

    //std::cerr << "table row count = " << M_model->rowCount() << std::endl;

    layout->addWidget( M_item_view );
    this->setLayout( layout );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
PlayerTypeDialog::createModel()
{
    const int ROW_SIZE = M_disp_holder.playerTypes().size();

    //M_model = new QStandardItemModel( ROW_SIZE, 15, this );
    M_model = new QStandardItemModel( ROW_SIZE, 16, this );

    int i = 0;
    M_model->setHeaderData( i, Qt::Horizontal, tr( "id" ) ); ++i;
    M_model->setHeaderData( i, Qt::Horizontal, tr( "Size" ) ); ++i;
    M_model->setHeaderData( i, Qt::Horizontal, tr( "Speed Real/Max" ) ); ++i;
    M_model->setHeaderData( i, Qt::Horizontal, tr( "AccMax" ) ); ++i;
    M_model->setHeaderData( i, Qt::Horizontal, tr( "DashRate" ) ); ++i;
    M_model->setHeaderData( i, Qt::Horizontal, tr( "Decay" ) ); ++i;
    M_model->setHeaderData( i, Qt::Horizontal, tr( "IMoment" ) ); ++i;
    M_model->setHeaderData( i, Qt::Horizontal, tr( "Kickable" ) ); ++i;
    M_model->setHeaderData( i, Qt::Horizontal, tr( "KickMargin" ) ); ++i;
    M_model->setHeaderData( i, Qt::Horizontal, tr( "KickRate" ) ); ++i;
    M_model->setHeaderData( i, Qt::Horizontal, tr( "KickRand" ) ); ++i;
    M_model->setHeaderData( i, Qt::Horizontal, tr( "UnrelCatch" ) ); ++i;
    M_model->setHeaderData( i, Qt::Horizontal, tr( "StamInc" ) ); ++i;
    M_model->setHeaderData( i, Qt::Horizontal, tr( "ExtStam" ) ); ++i;
    M_model->setHeaderData( i, Qt::Horizontal, tr( "Eft Max-Min" ) ); ++i;
    M_model->setHeaderData( i, Qt::Horizontal, tr( "FoulProb" ) ); ++i;

    updateData();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
PlayerTypeDialog::adjustSize()
{
    QFontMetrics metrics = M_item_view->fontMetrics();

    //M_item_view->resizeRowsToContents();
    const int row_height = metrics.height() + 2;
    const int ROW_SIZE = M_disp_holder.playerTypes().size();
    for ( int i = 0; i < ROW_SIZE; ++i )
    {
        M_item_view->setRowHeight( i, row_height );
    }

    int i = 0;
    // id
    M_item_view->setColumnWidth( i, metrics.width( "   1" ) + 4 ); ++i;
    // size
    M_item_view->setColumnWidth( i, metrics.width( "  0.00" ) + 4 ); ++i;
    // speed max
    M_item_view->setColumnWidth( i, metrics.width( "00.000 / 00.000" ) + 4 ); ++i;
    // accel max
    M_item_view->setColumnWidth( i, metrics.width( "  0.000000" ) + 4 ); ++i;
    // dash power rate
    M_item_view->setColumnWidth( i, metrics.width( "  0.000000" ) + 4 ); ++i;
    // decay
    M_item_view->setColumnWidth( i, metrics.width( "  0.0000" ) + 4 ); ++i;
    // inertia
    M_item_view->setColumnWidth( i, metrics.width( "  0.0000" ) + 4 ); ++i;
    // kickable area
    M_item_view->setColumnWidth( i, metrics.width( "  0.0000" ) + 4 ); ++i;
    // kickable margin
    M_item_view->setColumnWidth( i, metrics.width( "  0.0000" ) + 4 ); ++i;
    // kick power rate
    M_item_view->setColumnWidth( i, metrics.width( "  0.000000" ) + 4 ); ++i;
    // kick rand
    M_item_view->setColumnWidth( i, metrics.width( "  0.0000" ) + 4 ); ++i;
    // catchable area
    M_item_view->setColumnWidth( i, metrics.width( "  0.000 - 0.000" ) + 4 ); ++i;
    // stamina inc max
    M_item_view->setColumnWidth( i, metrics.width( "  00.00" ) + 4 ); ++i;
    // extra stamina
    M_item_view->setColumnWidth( i, metrics.width( "  00.00" ) + 4 ); ++i;
    // effort max - min
    M_item_view->setColumnWidth( i, metrics.width( "  0.000 - 0.000" ) + 4 ); ++i;
    // foul detect probability
    M_item_view->setColumnWidth( i, metrics.width( "  0.0000" ) + 4 ); ++i;

    QRect rect = this->geometry();
    QRect child_rect = this->childrenRect();
    int width_diff = rect.width() - child_rect.width();
    int height_diff = rect.height() - child_rect.height();

    rect.setWidth( M_item_view->horizontalHeader()->length()
                   + width_diff );
    rect.setHeight( M_item_view->horizontalHeader()->height()
                    + M_item_view->rowHeight( 0 ) * ROW_SIZE
                    + height_diff );

    this->setGeometry( rect );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
PlayerTypeDialog::updateData()
{
    const int ROW_SIZE = M_disp_holder.playerTypes().size();

    if ( M_model->rowCount() != ROW_SIZE )
    {
#if QT_VERSION >= 0x040200
        M_model->setRowCount( ROW_SIZE );
#else
        if ( M_model->rowCount() < ROW_SIZE )
        {
            M_model->insertRows( 0, ROW_SIZE - M_model->rowCount() );
        }
        else if ( M_model->rowCount() > ROW_SIZE )
        {
            M_model->removeRows( 0, M_model->rowCount() - ROW_SIZE );
        }
#endif
    }

    const rcss::rcg::ServerParamT & SP = M_disp_holder.serverParam();

    QString text;
    for ( int row = 0; row < ROW_SIZE; ++row )
    {
        const std::map< int , rcss::rcg::PlayerTypeT >::const_iterator it
            = M_disp_holder.playerTypes().find( row );
        if ( it == M_disp_holder.playerTypes().end() )
        {
            continue;
        }

        const rcss::rcg::PlayerTypeT & param = it->second;

        int i = 0;

        // id
        M_model->setData( M_model->index( row, i ),
                          param.id_ );
        ++i;

        // size
        //text.sprintf( "%.2f", param.player_size_ );
        text = QString::number( param.player_size_ );
        M_model->setData( M_model->index( row, i ), text ); ++i;

        // speed real/max
        double accel_max = SP.max_power_ * param.dash_power_rate_ * param.effort_max_;
        double real_speed_max = accel_max / ( 1.0 - param.player_decay_ );
        if ( real_speed_max > param.player_speed_max_ )
        {
            real_speed_max = param.player_speed_max_;
        }

        text.sprintf( "%5.3f / %5.3f",
                      real_speed_max, param.player_speed_max_ );
        M_model->setData( M_model->index( row, i ), text ); ++i;

        // accel max
        //text.sprintf( "%.4f", accel_max );
        text = QString::number( accel_max );
        M_model->setData( M_model->index( row, i ), text ); ++i;

        // dash power rate
        //text.sprintf( "%.5f", param.dash_power_rate_ );
        text = QString::number( param.dash_power_rate_ );
        M_model->setData( M_model->index( row, i ), text ); ++i;

        // decay
        //text.sprintf( "%.3f", param.player_decay_ );
        text = QString::number( param.player_decay_ );
        M_model->setData( M_model->index( row, i ), text ); ++i;

        // inertia
        //text.sprintf( "%.2f", param.inertia_moment_ );
        text = QString::number( param.inertia_moment_ );
        M_model->setData( M_model->index( row, i ), text ); ++i;

        // kickable area
        //text.sprintf( "%.3f", param.player_size_ + param.kickable_margin_ + SP.ball_size_ );
        text = QString::number( param.player_size_ + param.kickable_margin_ + SP.ball_size_ );
        M_model->setData( M_model->index( row, i ), text ); ++i;

        // kickable margin
        //text.sprintf( "%.3f", param.kickable_margin_ );
        text = QString::number( param.kickable_margin_ );
        M_model->setData( M_model->index( row, i ), text ); ++i;

        // kick power rate
        //text.sprintf( "%.6f", param.kick_power_rate_ );
        text = QString::number( param.kick_power_rate_ );
        M_model->setData( M_model->index( row, i ), text ); ++i;

        // kick rand
        //text.sprintf( "%.2f", param.kick_rand_ );
        text = QString::number( param.kick_rand_ );
        M_model->setData( M_model->index( row, i ), text ); ++i;

        // catch area radius
        double min_r = std::sqrt( std::pow( SP.catchable_area_w_ * 0.5, 2.0 )
                                  + std::pow( SP.catchable_area_l_ * ( 1.0 - ( param.catchable_area_l_stretch_ - 1.0 ) ), 2.0 ) );
        double max_r = std::sqrt( std::pow( SP.catchable_area_w_ * 0.5, 2.0 )
                                  + std::pow( SP.catchable_area_l_ * param.catchable_area_l_stretch_, 2.0 ) );
        text.sprintf( "%.3f - %.3f", min_r, max_r );
        M_model->setData( M_model->index( row, i ), text ); ++i;

        // stamina inc max
        //text.sprintf( "%.2f", param.stamina_inc_max_ );
        text = QString::number( param.stamina_inc_max_ );
        M_model->setData( M_model->index( row, i ), text ); ++i;

        // extra stamina
        //text.sprintf( "%.2f", param.extra_stamina_ );
        text = QString::number( param.extra_stamina_ );
        M_model->setData( M_model->index( row, i ), text ); ++i;

        // effort max - min
        text.sprintf( "%.3f - %.3f",
                      param.effort_max_, param.effort_min_ );
        M_model->setData( M_model->index( row, i ), text ); ++i;

        // foul detect probability
        //text.sprintf( "%.4f", param.foul_detect_probability_ );
        text = QString::number( param.foul_detect_probability_ );
        M_model->setData( M_model->index( row, i ), text ); ++i;
    }

    adjustSize();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
PlayerTypeDialog::showEvent( QShowEvent * event )
{
    updateData();

    event->accept();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
PlayerTypeDialog::wheelEvent( QWheelEvent * event )
{

    if ( event->delta() < 0 )
    {
        this->setWindowOpacity( std::max( 0.1, this->windowOpacity() - 0.05 ) );
    }
    else
    {
        this->setWindowOpacity( std::min( 1.0, this->windowOpacity() + 0.05 ) );
    }

    event->accept();
}
