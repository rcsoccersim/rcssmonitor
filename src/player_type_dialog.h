// -*-c++-*-

/*!
  \file player_type_dialog.h
  \brief player type list dialog class Header File.
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

#ifndef RCSSMONITOR_PLAYER_TYPE_DIALOG_H
#define RCSSMONITOR_PLAYER_TYPE_DIALOG_H

#include <QDialog>

class QAbstractItemModel;
class QShowEvent;
class QStandardItemModel;
class QTableView;
class QWheelEvent;

class DispHolder;

//! movable objects detail info dialog
class PlayerTypeDialog
    : public QDialog {

    Q_OBJECT

private:

    const DispHolder & M_disp_holder;

    QStandardItemModel * M_model;
    QTableView * M_item_view;

    // not used
    PlayerTypeDialog();
    PlayerTypeDialog( const PlayerTypeDialog & );
    const PlayerTypeDialog & operator=( const PlayerTypeDialog & );

public:
    //! constructor
    PlayerTypeDialog( const DispHolder & disp_holder,
                      QWidget * parent );


    ~PlayerTypeDialog();

private:

    void createTable();
    void createModel();

    void adjustSize();

protected:

    void showEvent( QShowEvent * event );
    void wheelEvent( QWheelEvent * event );

public slots:

    void updateData();

};

#endif
