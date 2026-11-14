// -*-c++-*-

/*!
  \file config_dialog.cpp
  \brief Config Dialog class Source File.
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

#include "config_dialog.h"

#include "disp_holder.h"
#include "options.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <cassert>


/*-------------------------------------------------------------------*/
/*!

*/
ColorItem::ColorItem( const QString & name,
                      const QColor & old_color,
                      Setter setter,
                      QListWidget * parent )
    : QListWidgetItem( name, parent ),
      M_new_color( old_color ),
      M_old_color( old_color ),
      M_setter( setter )
{
    QPixmap pixmap( 16, 16 );
    pixmap.fill( old_color );
    this->setIcon( QIcon( pixmap ) );
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
ColorItem::setColor( const QColor & color )
{
    if ( M_new_color != color )
    {
        M_new_color = color;

        if ( M_setter )
        {
            M_setter( color );
        }

        QPixmap pixmap( 16, 16 );
        pixmap.fill( color );
        this->setIcon( QIcon( pixmap ) );

        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ColorItem::updateColor()
{
    M_old_color = M_new_color;
}

/*-------------------------------------------------------------------*/
/*!

*/
bool
ColorItem::revert()
{
    return setColor( M_old_color );
}

/*-------------------------------------------------------------------*/
/*!

*/
FontButton::FontButton( const QString & name,
                        const QFont & old_font,
                        Setter setter,
                        ConfigDialog * parent )
    : QPushButton( parent )
    , M_name( name )
    , M_new_font( old_font )
    , M_old_font( old_font )
    , M_setter( setter )
{
    QFont button_font = old_font;
    button_font.setPointSize( QApplication::font().pointSize() );
    this->setFont( button_font );

    updateText();

    connect( this, SIGNAL( clicked() ),
             this, SLOT( fontDialog() ) );
    connect( this, SIGNAL( fontChanged() ),
             parent, SIGNAL( configured() ) );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FontButton::setNewFont( const QFont & font )
{
    if ( M_new_font != font )
    {
        if ( M_setter )
        {
            M_setter( font );
        }

        M_new_font = font;

        QFont button_font = font;
        button_font.setPointSize( QApplication::font().pointSize() );
        this->setFont( button_font );

        updateText();

        emit fontChanged();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FontButton::fontDialog()
{
    bool ok = true;
    QFont font = QFontDialog::getFont( &ok, M_new_font );
    if ( ok )
    {
        setNewFont( font );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FontButton::updateText()
{
    QString str = M_name;
    str += tr( " (" );
    str += M_new_font.family();
    str += tr( "," );
    str += QString::number( M_new_font.pointSize() );
    str += tr( ")" );
    this->setText( str );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
FontButton::revert()
{
    setNewFont( M_old_font );
}


/*-------------------------------------------------------------------*/
/*-------------------------------------------------------------------*/
/*-------------------------------------------------------------------*/


/*-------------------------------------------------------------------*/
/*!

*/
ConfigDialog::ConfigDialog( QWidget * parent,
                            const DispHolder & disp_holder )
    : QDialog( parent )
    , M_disp_holder( disp_holder )
{
    this->setWindowTitle( tr( "Config" ) );

    createWidgets();
}

/*-------------------------------------------------------------------*/
/*!

*/
ConfigDialog::~ConfigDialog()
{
    //std::cerr << "delete ConfigDialog" << std::endl;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::createWidgets()
{
    M_tab_widget = new QTabWidget();
    QVBoxLayout * top_layout = new QVBoxLayout();

    // show
    {
        QFrame * frame = new QFrame();
        QVBoxLayout * layout = new QVBoxLayout();
        layout->setSizeConstraint( QLayout::SetFixedSize );
        layout->setContentsMargins( 4, 4, 4, 4 );
        layout->setSpacing( 4 );

        layout->addWidget( createShowControls(),
                           0, Qt::AlignLeft );
        layout->addWidget( createPlayerInfoControls(),
                           0, Qt::AlignLeft );
        layout->addWidget( createPlayerSelectionControls(),
                           0, Qt::AlignLeft );
        layout->addWidget( createBallInfoControls(),
                           0, Qt::AlignLeft );

        layout->addStretch();

        frame->setLayout( layout );
        M_tab_widget->addTab( frame, tr( "Show" ) );
    }
    // canvas
    {
        QFrame * frame = new QFrame();
        QVBoxLayout * layout = new QVBoxLayout();
        layout->setSizeConstraint( QLayout::SetFixedSize );
        layout->setContentsMargins( 4, 4, 4, 4 );
        layout->setSpacing( 4 );

        layout->addWidget( createZoomControls(),
                           0, Qt::AlignLeft );
        layout->addWidget( createFocusControls(),
                           0, Qt::AlignLeft );
        layout->addWidget( createObjectSizeControls(),
                           0, Qt::AlignLeft );
        layout->addWidget( createCanvasSizeControls(),
                           0, Qt::AlignLeft );
        layout->addWidget( createGridStepControls(),
                           0, Qt::AlignLeft );
        layout->addWidget( createMiscControls(),
                           0, Qt::AlignLeft );

        layout->addStretch();

        frame->setLayout( layout );
        M_tab_widget->addTab( frame, tr( "Canvas" ) );
    }
    // color
    {
        QFrame * frame = new QFrame();
        QLayout * layout = createColorList();
        layout->setSizeConstraint( QLayout::SetFixedSize );
        layout->setContentsMargins( 4, 4, 4, 4 );
        layout->setSpacing( 4 );

        frame->setLayout( layout );
        M_tab_widget->addTab( frame, tr( "Color" ) );
    }
    // font
    {
        QFrame * frame = new QFrame();
        QLayout * layout = createFontButtons();
        layout->setSizeConstraint( QLayout::SetFixedSize );
        layout->setContentsMargins( 4, 4, 4, 4 );
        layout->setSpacing( 4 );

        frame->setLayout( layout );
        M_tab_widget->addTab( frame, tr( "Font" ) );
    }
#ifdef HAVE_3D_VIEW
    // the 3D view
    M_tab_widget->addTab( create3DTab(), tr( "3D" ) );
    M_tab_widget->addTab( createPlayerCameraTab(), tr( "Player Camera" ) );
#endif

    // object selection
//     {
//         QFrame * frame = new QFrame();
//         QVBoxLayout * layout = new QVBoxLayout();
//         layout->setSizeConstraint( QLayout::SetFixedSize );
//         layout->setContentsMargins( 4, 4, 4, 4 );
//         layout->setSpacing( 4 );
//
//         layout->addWidget( createTraceControls(),
//                            0, Qt::AlignLeft );
//         layout->addWidget( createInertiaMoveControls(),
//                            0, Qt::AlignLeft );
//
//         frame->setLayout( layout );
//         M_tab_widget->addTab( frame, tr( "Object Selection" ) );
//     }

    top_layout->setSizeConstraint( QLayout::SetFixedSize );
    top_layout->addWidget( M_tab_widget );
    this->setLayout( top_layout );
}

/*-------------------------------------------------------------------*/
/*!

*/
QWidget *
ConfigDialog::createZoomControls()
{
    QGroupBox * group_box = new QGroupBox( tr( "Zoom" ) );

    QVBoxLayout * top_layout = new QVBoxLayout();
    top_layout->setSizeConstraint( QLayout::SetFixedSize );
    top_layout->setContentsMargins( 1, 1, 1, 1 );
    top_layout->setSpacing( 0 );

    const QFontMetrics metrics = this->fontMetrics();
    const int button_height = metrics.height() + 12;

    {
        QHBoxLayout * layout = new QHBoxLayout();
        layout->setSpacing( 0 );

        QPushButton * zoomin_button = new QPushButton( tr( "+" ) );
        zoomin_button->setMaximumSize( 48, button_height );
        zoomin_button->setToolTip( tr( "Z" ) );
        connect( zoomin_button, SIGNAL( clicked() ),
                 this, SLOT( zoomIn() ) );
        layout->addWidget( zoomin_button );
        //
        QPushButton * zoomout_button = new QPushButton( tr( "-" ) );
        zoomout_button->setMaximumSize( 48, button_height );
        connect( zoomout_button, SIGNAL( clicked() ),
                 this, SLOT( zoomOut() ) );
        layout->addWidget( zoomout_button );
        //
        layout->addSpacing( 1 );
        layout->addStretch( 1 );
        //
        QPushButton * fit_button = new QPushButton( tr( "Fit" ) );
        fit_button->setMaximumSize( 60, button_height );
        connect( fit_button, SIGNAL( clicked() ),
                 this, SLOT( fitToScreen() ) );
        layout->addWidget( fit_button );

        top_layout->addLayout( layout );
    }

    {
        QHBoxLayout * layout = new QHBoxLayout();
        layout->setSpacing( 0 );

        layout->addWidget( new QLabel( tr( "Scale:" ) ) );

        layout->addSpacing( 2 );

        M_scale_slider = new QSlider( Qt::Horizontal );
        M_scale_slider->setRange( static_cast< int >( Options::MIN_FIELD_SCALE * 10.0 ),
                                  static_cast< int >( Options::MAX_FIELD_SCALE * 10.0 ) );
        M_scale_slider->setValue( 80 );
        M_scale_slider->setSingleStep( 1 );
        M_scale_slider->setPageStep( 1 ); // set the minimal increment/decrement step
        M_scale_slider->setMinimumSize( 250, 24 );
        connect( M_scale_slider, SIGNAL( sliderMoved( int ) ),
                 this, SLOT( slideFieldScale( int ) ) );
        layout->addWidget( M_scale_slider );

        layout->addSpacing( 2 );

        M_scale_text = new QLineEdit( tr( "0.0" ) );
        M_scale_text->setValidator( new QDoubleValidator( 0.0, 400.0, 3, M_scale_text ) );
        M_scale_text->setMaximumSize( 48, 24 );
        connect( M_scale_text, SIGNAL( textChanged( const QString & ) ),
                 this, SLOT( editFieldScale( const QString & ) ) );
        layout->addWidget( M_scale_text );

        top_layout->addLayout( layout );
    }


    group_box->setLayout( top_layout );
    return group_box;;
}

/*-------------------------------------------------------------------*/
/*!

*/
QWidget *
ConfigDialog::createObjectSizeControls()
{
    QGroupBox * group_box = new QGroupBox( tr( "Object Size" ) );

    QVBoxLayout * top_layout = new QVBoxLayout();
    top_layout->setContentsMargins( 1, 1, 1, 1 );
    top_layout->setSpacing( 0 );


    QHBoxLayout * box = new QHBoxLayout();
    //
    box->addWidget( new QLabel( tr( "Ball:" ) ) );
    //
    M_ball_size_text = new QLineEdit( tr( "0.35" ) );
    M_ball_size_text->setValidator( new QDoubleValidator( 0.01, 100.0, 3, M_ball_size_text ) );
    M_ball_size_text->setMaximumSize( 48, 24 );
    connect( M_ball_size_text, SIGNAL( textChanged( const QString & ) ),
             this, SLOT( editBallSize( const QString & ) ) );
    box->addWidget( M_ball_size_text );
    //
    box->addWidget( new QLabel( tr( " Player:" ) ) );
    //
    M_player_size_text = new QLineEdit( tr( "0.0" ) );
    M_player_size_text->setValidator( new QDoubleValidator( 0.0, 100.0, 3, M_player_size_text ) );
    M_player_size_text->setMaximumSize( 48, 24 );
    connect( M_player_size_text, SIGNAL( textChanged( const QString & ) ),
             this, SLOT( editPlayerSize( const QString & ) ) );
    box->addWidget( M_player_size_text );
    //
    box->addWidget( new QLabel( tr( " Focus Point:" ) ) );
    //
    M_focus_point_size_text = new QLineEdit( tr( "2.0" ) );
    M_focus_point_size_text->setValidator( new QDoubleValidator( 0.1, 100.0, 3, M_focus_point_size_text ) );
    M_focus_point_size_text->setMaximumSize( 48, 24 );
    connect( M_focus_point_size_text, SIGNAL( textChanged( const QString & ) ),
             this, SLOT( editFocusPointSize( const QString & ) ) );
    box->addWidget( M_focus_point_size_text );
    //
    top_layout->addLayout( box );

    group_box->setLayout( top_layout );
    return group_box;
}

/*-------------------------------------------------------------------*/
/*!

*/
QWidget *
ConfigDialog::createCanvasSizeControls()
{
    QGroupBox * group_box = new QGroupBox( tr( "Canvas Size" ) );
    QVBoxLayout * top_layout = new QVBoxLayout();

    {
        QHBoxLayout * layout = new QHBoxLayout();
        layout->setContentsMargins( 1, 1, 1, 1 );
        layout->setSpacing( 0 );

        layout->addWidget( new QLabel( tr( " Width:" ) ) );

        M_canvas_width_text = new QLineEdit( tr( "640" ) );
        M_canvas_width_text->setValidator( new QIntValidator( 100, 3000, M_canvas_width_text ) );
        M_canvas_width_text->setMaximumSize( 48, 24 );
        layout->addWidget( M_canvas_width_text );

        layout->addWidget( new QLabel( tr( " Height:" ) ) );

        M_canvas_height_text = new QLineEdit( tr( "480" ) );
        M_canvas_height_text->setValidator( new QIntValidator( 100, 3000, M_canvas_height_text ) );
        M_canvas_height_text->setMaximumSize( 48, 24 );
        layout->addWidget( M_canvas_height_text );

        layout->addSpacing( 12 );

        QPushButton * apply_canvas_size_btn = new QPushButton( tr( "apply" ) );
        apply_canvas_size_btn->setMaximumSize( 60, this->fontMetrics().height() + 12 );
        connect( apply_canvas_size_btn, SIGNAL( clicked() ),
                 this, SLOT( applyCanvasSize() ) );
        layout->addWidget( apply_canvas_size_btn );

        top_layout->addLayout( layout );
    }
    {
        QHBoxLayout * layout = new QHBoxLayout();
        layout->setContentsMargins( 1, 1, 1, 1 );
        layout->setSpacing( 0 );

        layout->addWidget( new QLabel( tr( " Team Graphic Scale:" ) ) );

        M_team_graphic_scale = new QDoubleSpinBox();
        M_team_graphic_scale->setRange( 0.01, 16.0 );
        M_team_graphic_scale->setSingleStep( 0.01 );
        M_team_graphic_scale->setMaximumSize( 128, 24 );
        connect( M_team_graphic_scale, SIGNAL( valueChanged( double ) ),
                 this, SLOT( slotTeamGraphicScaleChanged( double ) ) );
        layout->addWidget( M_team_graphic_scale );

        top_layout->addLayout( layout );
    }
    group_box->setLayout( top_layout );
    return group_box;
}

/*-------------------------------------------------------------------*/
/*!

*/
QWidget *
ConfigDialog::createPlayerInfoControls()
{
    QGroupBox * group_box = new QGroupBox( tr( "Player Info" ) );

    QVBoxLayout * top_layout = new QVBoxLayout();
    top_layout->setContentsMargins( 1, 1, 1, 1 );
    top_layout->setSpacing( 0 );

    {
        QHBoxLayout * layout = new QHBoxLayout();
        layout->setContentsMargins( 0, 0, 0, 0 );
        layout->setSpacing( 0 );

        M_player_number_cb = new QCheckBox( tr( "Unum" ) );
        M_player_number_cb->setChecked( Options::instance().showPlayerNumber() );
        connect( M_player_number_cb, SIGNAL( clicked( bool ) ),
                 this, SLOT( clickShowPlayerNumber( bool ) ) );
        layout->addWidget( M_player_number_cb );
        //
        M_player_type_cb = new QCheckBox( tr( "Type" ) );
        M_player_type_cb->setChecked( Options::instance().showPlayerType() );
        connect( M_player_type_cb, SIGNAL( clicked( bool ) ),
                 this, SLOT( clickShowPlayerType( bool ) ) );
        layout->addWidget( M_player_type_cb );

        //
        M_card_cb = new QCheckBox( tr( "Card" ) );
        M_card_cb->setChecked( Options::instance().showCard() );
        connect( M_card_cb, SIGNAL( clicked( bool ) ),
                 this, SLOT( clickShowCard( bool ) ) );
        layout->addWidget( M_card_cb );

        //
        M_illegal_defense_cb = new QCheckBox( tr( "Illegal Defense" ) );
        M_illegal_defense_cb->setChecked( Options::instance().showIllegalDefense() );
        connect( M_illegal_defense_cb, SIGNAL( clicked( bool ) ),
                 this, SLOT( clickShowIllegalDefense( bool ) ) );
        layout->addWidget( M_illegal_defense_cb );

        top_layout->addLayout( layout );
    }
    {
        QHBoxLayout * layout = new QHBoxLayout();
        layout->setContentsMargins( 0, 0, 0, 0 );
        layout->setSpacing( 0 );

        //
        M_stamina_cb = new QCheckBox( tr( "Stamina" ) );
        M_stamina_cb->setChecked( Options::instance().showStamina() );
        connect( M_stamina_cb, SIGNAL( clicked( bool ) ),
                 this, SLOT( clickShowStamina( bool ) ) );
        layout->addWidget( M_stamina_cb );
        //
        M_stamina_capacity_cb = new QCheckBox( tr( "Stamina Capacity" ) );
        M_stamina_capacity_cb->setChecked( Options::instance().showStaminaCapacity() );
        connect( M_stamina_capacity_cb, SIGNAL( clicked( bool ) ),
                 this, SLOT( clickShowStaminaCapacity( bool ) ) );
        layout->addWidget( M_stamina_capacity_cb );

        top_layout->addLayout( layout );
    }
    {
        QHBoxLayout * layout = new QHBoxLayout();
        layout->setContentsMargins( 0, 0, 0, 0 );
        layout->setSpacing( 0 );

        //
        M_view_area_cb = new QCheckBox( tr( "View Area" ) );
        M_view_area_cb->setChecked( Options::instance().showViewArea() );
        connect( M_view_area_cb, SIGNAL( clicked( bool ) ),
                 this, SLOT( clickShowViewArea( bool ) ) );
        layout->addWidget( M_view_area_cb );

        //
        M_focus_point_cb = new QCheckBox( tr( "Focus Point" ) );
        M_focus_point_cb->setChecked( Options::instance().showFocusPoint() );
        connect( M_focus_point_cb, SIGNAL( clicked( bool ) ),
                 this, SLOT( clickShowFocusPoint( bool ) ) );
        layout->addWidget( M_focus_point_cb );

        // the other players' focus points too, with Focus Point
        M_all_focus_points_cb = new QCheckBox( tr( "All Focus Points" ) );
        M_all_focus_points_cb->setToolTip( tr( "With Focus Point, show the focus points of all players,\n"
                                               "smaller and in their team colors. The selected player's\n"
                                               "focus point has the line and the lines of the same noise." ) );
        M_all_focus_points_cb->setChecked( Options::instance().showAllFocusPoints() );
        M_all_focus_points_cb->setEnabled( Options::instance().showFocusPoint() );
        connect( M_all_focus_points_cb, SIGNAL( clicked( bool ) ),
                 this, SLOT( clickShowAllFocusPoints( bool ) ) );
        layout->addWidget( M_all_focus_points_cb );

        top_layout->addLayout( layout );
    }

    {
        QHBoxLayout * layout = new QHBoxLayout();
        layout->setContentsMargins( 0, 0, 0, 0 );
        layout->setSpacing( 0 );
        //
        M_catch_area_cb = new QCheckBox( tr( "Catch Area" ) );
        M_catch_area_cb->setChecked( Options::instance().showCatchArea() );
        connect( M_catch_area_cb, SIGNAL( clicked( bool ) ),
                 this, SLOT( clickShowCatchArea( bool ) ) );
        layout->addWidget( M_catch_area_cb );

        //
        M_tackle_area_cb = new QCheckBox( tr( "Tackle Area" ) );
        M_tackle_area_cb->setChecked( Options::instance().showTackleArea() );
        connect( M_tackle_area_cb, SIGNAL( clicked( bool ) ),
                 this, SLOT( clickShowTackleArea( bool ) ) );
        layout->addWidget( M_tackle_area_cb );

        top_layout->addLayout( layout );
    }
    {
        QHBoxLayout * layout = new QHBoxLayout();
        layout->setContentsMargins( 0, 0, 0, 0 );
        layout->setSpacing( 0 );

        //
        M_kick_accel_area_cb = new QCheckBox( tr( "Kick Accel Area" ) );
        M_kick_accel_area_cb->setChecked( Options::instance().showKickAccelArea() );
        connect( M_kick_accel_area_cb, SIGNAL( clicked( bool ) ),
                 this, SLOT( clickShowKickAccelArea( bool ) ) );
        layout->addWidget( M_kick_accel_area_cb );

        //
        M_pointto_cb = new QCheckBox( tr( "Pointto" ) );
        M_pointto_cb->setChecked( Options::instance().showPointto() );
        connect( M_pointto_cb, SIGNAL( clicked( bool ) ),
                 this, SLOT( clickShowPointto( bool ) ) );
        layout->addWidget( M_pointto_cb );

        top_layout->addLayout( layout );
    }
//     {
//         QHBoxLayout * layout = new QHBoxLayout();
//         layout->setContentsMargins( 0, 0, 0, 0 );
//         layout->setSpacing( 0 );

//         top_layout->addLayout( layout );
//     }

    group_box->setLayout( top_layout );
    return group_box;
}

/*-------------------------------------------------------------------*/
/*!

*/
QWidget *
ConfigDialog::createBallInfoControls()
{
    QGroupBox * group_box = new QGroupBox( tr( "Ball Info" ) );

    QHBoxLayout * top_layout = new QHBoxLayout();
    top_layout->setContentsMargins( 1, 1, 1, 1 );
    top_layout->setSpacing( 0 );

    top_layout->addWidget( new QLabel( tr( "Velocity Cycles:" ) ) );

    top_layout->addSpacing( 2 );

    M_ball_vel_cycle = new QSpinBox();
    //M_ball_vel_cycle->setMinimumSize( 60, 24 );
    M_ball_vel_cycle->setValue( 0 );
    M_ball_vel_cycle->setRange( 0, 50 );
    connect( M_ball_vel_cycle, SIGNAL( valueChanged( int ) ),
             this, SLOT( changeBallVelCycle( int ) ) );
    top_layout->addWidget( M_ball_vel_cycle );

    group_box->setLayout( top_layout );
    return group_box;
}

/*-------------------------------------------------------------------*/
/*!

*/
QWidget *
ConfigDialog::createShowControls()
{
    QGroupBox * group_box = new QGroupBox( tr( "Show" ) );

    QVBoxLayout * top_layout = new QVBoxLayout();
    top_layout->setContentsMargins( 1, 1, 1, 1 );
    top_layout->setSpacing( 0 );

    {
        QHBoxLayout * layout = new QHBoxLayout();
        layout->setContentsMargins( 0, 0, 0, 0 );
        layout->setSpacing( 0 );

        M_show_score_board_cb = new QCheckBox( tr( "Score Board" ) );
        M_show_score_board_cb->setChecked( Options::instance().showScoreBoard() );
        connect( M_show_score_board_cb, SIGNAL( clicked( bool ) ),
                 this, SLOT( clickShowScoreBoard( bool ) ) );
        layout->addWidget( M_show_score_board_cb );
        //
        M_show_team_graphic_cb = new QCheckBox( tr( "Team Graphic" ) );
        M_show_team_graphic_cb->setChecked( Options::instance().showTeamGraphic() );
        connect( M_show_team_graphic_cb, SIGNAL( clicked( bool ) ),
                 this, SLOT( clickShowTeamGraphic( bool ) ) );
        layout->addWidget( M_show_team_graphic_cb );

        top_layout->addLayout( layout );
    }
    {
        QHBoxLayout * layout = new QHBoxLayout();
        layout->setContentsMargins( 0, 0, 0, 0 );
        layout->setSpacing( 0 );

        //
        M_show_keepaway_area_cb = new QCheckBox( tr( "Keepaway Area" ) );
        M_show_keepaway_area_cb->setChecked( Options::instance().showKeepawayArea() );
        connect( M_show_keepaway_area_cb, SIGNAL( clicked( bool ) ),
                 this, SLOT( clickShowKeepawayArea( bool ) ) );
        layout->addWidget( M_show_keepaway_area_cb );
        //
        M_show_flag_cb = new QCheckBox( tr( "Flag" ) );
        M_show_flag_cb->setChecked( Options::instance().showFlag() );
        connect( M_show_flag_cb, SIGNAL( clicked( bool ) ),
                 this, SLOT( clickShowFlag( bool ) ) );
        layout->addWidget( M_show_flag_cb );

        top_layout->addLayout( layout );
    }
    {
        QHBoxLayout * layout = new QHBoxLayout();
        layout->setContentsMargins( 0, 0, 0, 0 );
        layout->setSpacing( 0 );

        M_show_ball_cb = new QCheckBox( tr( "Ball" ) );
        M_show_ball_cb->setChecked( Options::instance().showBall() );
        connect( M_show_ball_cb, SIGNAL( clicked( bool ) ),
                 this, SLOT( clickShowBall( bool ) ) );
        layout->addWidget( M_show_ball_cb );
        //
        M_show_player_cb = new QCheckBox( tr( "Player" ) );
        M_show_player_cb->setChecked( Options::instance().showPlayer() );
        connect( M_show_player_cb, SIGNAL( clicked( bool ) ),
                 this, SLOT( clickShowPlayer( bool ) ) );
        layout->addWidget( M_show_player_cb );
        //
        M_show_offside_line_cb = new QCheckBox( tr( "Offside Line" ) );
        M_show_offside_line_cb->setChecked( Options::instance().showOffsideLine() );
        connect( M_show_offside_line_cb, SIGNAL( clicked( bool ) ),
                 this, SLOT( clickShowOffsideLine( bool ) ) );
        layout->addWidget( M_show_offside_line_cb );
        //
        M_show_draw_info_cb = new QCheckBox( tr( "Draw Info" ) );
        M_show_draw_info_cb->setChecked( Options::instance().showDrawInfo() );
        connect( M_show_draw_info_cb, SIGNAL( clicked( bool ) ),
                 this, SLOT( clickShowDrawInfo( bool ) ) );
        layout->addWidget( M_show_draw_info_cb );

        top_layout->addLayout( layout );
    }
//     {
//         QHBoxLayout * layout = new QHBoxLayout();
//         layout->setContentsMargins( 0, 0, 0, 0 );
//         layout->setSpacing( 0 );

//         top_layout->addLayout( layout );
//     }

    group_box->setLayout( top_layout );
    return group_box;
}

/*-------------------------------------------------------------------*/
/*!

*/
QWidget *
ConfigDialog::createFocusControls()
{
    QGroupBox * group_box = new QGroupBox( tr( "Focus" ) );

    QHBoxLayout * layout = new QHBoxLayout();
    layout->setContentsMargins( 1, 1, 1, 1 );
    layout->setSpacing( 0 );

    {
        M_focus_ball_rb = new QRadioButton( tr( "Ball" ) );
        connect( M_focus_ball_rb, SIGNAL( clicked() ),
                 this, SLOT( clickFocusBall() ) );
        layout->addWidget( M_focus_ball_rb );
    }
    {
        M_focus_player_rb = new QRadioButton( tr( "Selected Player" ) );
        connect( M_focus_player_rb, SIGNAL( clicked() ),
                 this, SLOT( clickFocusPlayer() ) );
        layout->addWidget( M_focus_player_rb );
    }
    {
        M_focus_fix_rb = new QRadioButton( tr( "Fix" ) );
        connect( M_focus_fix_rb, SIGNAL( clicked() ),
                 this, SLOT( setFocusFix() ) );
        layout->addWidget( M_focus_fix_rb );
    }
    M_focus_fix_rb->setChecked( true );

    group_box->setLayout( layout );
    return group_box;
}

/*-------------------------------------------------------------------*/
/*!

*/
QWidget *
ConfigDialog::createPlayerSelectionControls()
{
    QGroupBox * group_box = new QGroupBox( tr( "Player Selection" ) );

    QVBoxLayout * top_layout = new QVBoxLayout();
    top_layout->setContentsMargins( 1, 1, 1, 1 );
    top_layout->setSpacing( 0 );

    // selection type
    {
        QHBoxLayout * layout = new QHBoxLayout();
        layout->setContentsMargins( 0, 0, 0, 0 );
        layout->setSpacing( 0 );
        {
            M_select_all_rb = new QRadioButton( tr( "Auto" ) );
            //M_select_all_rb->setToolTip( tr( "A" ) );
            connect( M_select_all_rb, SIGNAL( clicked() ),
                     this, SLOT( clickSelectAutoAll() ) );
            layout->addWidget( M_select_all_rb );
        }
        {
            M_select_left_rb = new QRadioButton( tr( "Auto left" ) );
            //M_select_left_rb->setToolTip( tr( "L" ) );
            connect( M_select_left_rb, SIGNAL( clicked() ),
                     this, SLOT( clickSelectAutoLeft() ) );
            layout->addWidget( M_select_left_rb );
        }
        {
            M_select_right_rb = new QRadioButton( tr( "Auto right" ) );
            //M_select_right_rb->setToolTip( tr( "R" ) );
            connect( M_select_right_rb, SIGNAL( clicked() ),
                     this, SLOT( clickSelectAutoRight() ) );
            layout->addWidget( M_select_right_rb );
        }
        {
            M_select_fix_rb = new QRadioButton( tr( "Fix" ) );
            connect( M_select_fix_rb, SIGNAL( clicked() ),
                     this, SLOT( clickSelectFix() ) );
            layout->addWidget( M_select_fix_rb );
        }
        {
            M_unselect_rb = new QRadioButton( tr( "Unselect" ) );
            //M_unselect_rb->setToolTip( tr( "U" ) );
            connect( M_unselect_rb, SIGNAL( clicked() ),
                     this, SLOT( setUnselect() ) );
            layout->addWidget( M_unselect_rb );
        }
        M_unselect_rb->setChecked( true );

        top_layout->addLayout( layout );
    }

    {
        QHBoxLayout * layout = new QHBoxLayout();
        layout->setContentsMargins( 0, 0, 0, 0 );
        layout->setSpacing( 0 );
        //layout->addSpacing( 16 );

        M_player_choice = new QComboBox();
        M_player_choice->addItem( tr ( "No Selection" ) );
        for ( int i = 1; i <= 11; ++i )
        {
            M_player_choice->addItem( QString( "Left number: %1" ).arg( i ) );
        }
        for ( int i = 1; i <= 11; ++i )
        {
            M_player_choice->addItem( QString( "Right number: %1" ).arg( i ) );
        }
        M_player_choice->setCurrentIndex( 0 );
        int id_width = this->fontMetrics().horizontalAdvance( tr( "Right Number: 11" ) );
        M_player_choice->setMaximumWidth( id_width + 40 );
        connect( M_player_choice, SIGNAL( currentIndexChanged( int ) ),
                 this, SLOT( choicePlayer( int ) ) );
        layout->addWidget( M_player_choice, 0, Qt::AlignLeft );

        layout->addSpacing( 16 );

        top_layout->addLayout( layout );
    }

    group_box->setLayout( top_layout );
    return group_box;
}

/*-------------------------------------------------------------------*/
/*!

*/
QWidget *
ConfigDialog::createMiscControls()
{
    QGroupBox * group_box = new QGroupBox( tr( "Misc" ) );
    QVBoxLayout * top_layout = new QVBoxLayout();
    top_layout->setContentsMargins( 1, 1, 1, 1 );
    top_layout->setSpacing( 0 );

    //
    M_anti_aliasing_cb = new QCheckBox( tr( "Anti Aliasing" ) );
    M_anti_aliasing_cb->setChecked( Options::instance().antiAliasing() );
    connect( M_anti_aliasing_cb, SIGNAL( toggled( bool ) ),
             this, SLOT( clickAntiAliasing( bool ) ) );
    top_layout->addWidget( M_anti_aliasing_cb );

    group_box->setLayout( top_layout );
    return group_box;
}

/*-------------------------------------------------------------------*/
/*!

*/
QWidget *
ConfigDialog::createGridStepControls()
{
    QGroupBox * group_box = new QGroupBox( tr( "Grid Lines" ) );
    QVBoxLayout * top_layout = new QVBoxLayout();
    top_layout->setContentsMargins( 1, 1, 1, 1 );
    top_layout->setSpacing( 0 );

    M_show_grid_coord_cb = new QCheckBox( tr( "Grid Coordinates" ) );
    M_show_grid_coord_cb->setChecked( Options::instance().showGridCoord() );
    connect( M_show_grid_coord_cb, SIGNAL( toggled( bool ) ),
             this, SLOT( clickShowGridCoord( bool ) ) );
    top_layout->addWidget( M_show_grid_coord_cb );

    //
    QHBoxLayout * layout = new QHBoxLayout();
    //layout->setContentsMargins( 0, 0, 0, 0 );
    //layout->setSpacing( 0 );
    //
    layout->addWidget( new QLabel( tr( "Step:" ) ) );
    //
    M_grid_step_slider = new QSlider( Qt::Horizontal );
    M_grid_step_slider->setRange( 0, 1000 );
    M_grid_step_slider->setValue( static_cast< int >( rint( Options::instance().gridStep() * 10.0 ) ) );
    M_grid_step_slider->setSingleStep( 1 );
    M_grid_step_slider->setPageStep( 1 );
    M_grid_step_slider->setMinimumSize( 260, 24 );
    connect( M_grid_step_slider, SIGNAL( sliderMoved( int ) ),
             this, SLOT( slideGridStep( int ) ) );
    layout->addWidget( M_grid_step_slider );

    //
    M_grid_step_text = new QLineEdit( tr( " 0.0" )  );
    M_grid_step_text->setValidator( new QDoubleValidator( 0.0, 100.0, 3, M_grid_step_text ) );
    M_grid_step_text->setMaximumSize( 48, 24 );
    M_grid_step_text->setText( QString::number( rint( Options::instance().gridStep() / 0.001 ) * 0.001 ) );
    connect( M_grid_step_text, SIGNAL( textChanged( const QString & ) ),
             this, SLOT( editGridStep( const QString & ) ) );
    layout->addWidget( M_grid_step_text );

    top_layout->addLayout( layout );
    group_box->setLayout( top_layout );
    return group_box;
}

/*-------------------------------------------------------------------*/
/*!

*/
#if 0
QWidget *
ConfigDialog::createTraceControls()
{
    QGroupBox * group_box = new QGroupBox( tr( "Trace" ) );

    QVBoxLayout * top_layout = new QVBoxLayout();
    top_layout->setContentsMargins( 0, 0, 0, 0 );
    top_layout->setSpacing( 0 );

    {
        QGroupBox * ball_box = new QGroupBox( tr( "Ball" ) );

        QHBoxLayout * layout = new QHBoxLayout();
        layout->setSizeConstraint( QLayout::SetFixedSize );
        layout->setContentsMargins( 0, 0, 0, 0 );
        layout->setSpacing( 0 );

        M_ball_trace_start = new QLineEdit( tr( "0" ) );
        M_ball_trace_start->setValidator( new QIntValidator( 0, 200000, M_ball_trace_start ) );
        M_ball_trace_start->setMaximumSize( 36, 24 );
        connect( M_ball_trace_start, SIGNAL( textEdited( const QString & ) ),
                 this, SLOT( editBallTraceStart( const QString & ) ) );
        layout->addWidget( M_ball_trace_start, 0, Qt::AlignVCenter );

        layout->addWidget( new QLabel( tr( "-" ) ), 0, Qt::AlignVCenter );

        M_ball_trace_end = new QLineEdit( tr( "0" ) );
        M_ball_trace_end->setValidator( new QIntValidator( 0, 200000, M_ball_trace_end ) );
        M_ball_trace_end->setMaximumSize( 36, 24 );
        connect( M_ball_trace_end, SIGNAL( textEdited( const QString & ) ),
                 this, SLOT( editBallTraceEnd( const QString & ) ) );
        layout->addWidget( M_ball_trace_end, 0, Qt::AlignVCenter );

        QPushButton * all_btn = new QPushButton( tr( "All" ) );
        all_btn->setMaximumSize( 48, this->fontMetrics().height() + 12 );
        all_btn->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
        connect( all_btn, SIGNAL( clicked() ),
                 this, SLOT( clickBallTraceAll() ) );
        layout->addWidget( all_btn, 0, Qt::AlignVCenter );

        ball_box->setLayout( layout );
        //first_layout->addWidget( ball_box );
        top_layout->addWidget( ball_box );
    }

    top_layout->addSpacing( 2 );
    {
        QGroupBox * player_box = new QGroupBox( tr( "Player" ) );

        QHBoxLayout * layout= new QHBoxLayout();
        layout->setSizeConstraint( QLayout::SetFixedSize );
        layout->setContentsMargins( 0, 0, 0, 0 );
        layout->setSpacing( 0 );

        M_player_trace_start = new QLineEdit( tr( "0" ) );
        M_player_trace_start->setValidator( new QIntValidator( 0, 200000, M_player_trace_start ) );
        M_player_trace_start->setMaximumSize( 36, 24 );
        connect( M_player_trace_start, SIGNAL( textEdited( const QString & ) ),
                 this, SLOT( editPlayerTraceStart( const QString & ) ) );
        layout->addWidget( M_player_trace_start, 0, Qt::AlignVCenter );

        layout->addWidget( new QLabel( tr( "-" ) ), 0, Qt::AlignVCenter );

        M_player_trace_end = new QLineEdit( tr( "0" ) );
        M_player_trace_end->setValidator( new QIntValidator( 0, 200000, M_player_trace_end ) );
        M_player_trace_end->setMaximumSize( 36, 24 );
        connect( M_ball_trace_end, SIGNAL( textEdited( const QString & ) ),
                 this, SLOT( editBallTraceEnd( const QString & ) ) );
        layout->addWidget( M_player_trace_end, 0, Qt::AlignVCenter );

        QPushButton * all_btn = new QPushButton( tr( "All" ) );
        all_btn->setMaximumSize( 48, this->fontMetrics().height() + 12 );
        all_btn->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
        connect( all_btn, SIGNAL( clicked() ),
                 this, SLOT( clickPlayerTraceAll() ) );
        layout->addWidget( all_btn, 0, Qt::AlignVCenter );

        player_box->setLayout( layout );
        //first_layout->addWidget( player_box );
        top_layout->addWidget( player_box );
    }

    top_layout->addSpacing( 2 );

    {
        QPushButton * line_point_btn = new QPushButton( tr( "Line/Point" ) );
        line_point_btn->setMaximumSize( 80, this->fontMetrics().height() + 12 );
        connect( line_point_btn, SIGNAL( clicked() ),
                 this, SLOT( clickLinePointButton() ) );

        top_layout->addWidget( line_point_btn, 0, Qt::AlignLeft );
    }

    group_box->setLayout( top_layout );
    return group_box;;
}

#endif

/*-------------------------------------------------------------------*/
/*!

*/
QLayout *
ConfigDialog::createColorList()
{
    QVBoxLayout * layout = new QVBoxLayout();

    M_color_list_box = new QListWidget();

    createColorItems();

    int font_width = this->fontMetrics().horizontalAdvance( tr( "Score Board Background" ) );
    M_color_list_box->setMinimumWidth( font_width + 16 + 10 );
    M_color_list_box->setMinimumHeight( 300 );
    connect( M_color_list_box, SIGNAL( itemClicked( QListWidgetItem * ) ),
             this, SLOT( selectColorEntry( QListWidgetItem * ) ) );

    layout->addWidget( M_color_list_box );

    //

    QHBoxLayout * hbox = new QHBoxLayout();

    QPushButton * default_button = new QPushButton( tr( "Default" ) );
    default_button->setAutoDefault( false );
    connect( default_button, SIGNAL( clicked() ),
             this, SLOT( setDefaultColor() ) );
    hbox->addWidget( default_button, 0, Qt::AlignLeft );
    //
    hbox->addStretch( 0 );
    //
    QPushButton * cancel_button = new QPushButton( tr( "Cancel" ) );
    cancel_button->setAutoDefault( false );
    connect( cancel_button, SIGNAL( clicked() ),
             this, SLOT( cancelColor() ) );
    hbox->addWidget( cancel_button, 0, Qt::AlignLeft );


    layout->addLayout( hbox );

    return layout;
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::createColorItems()
{
    using std::placeholders::_1;

    Options * o = &Options::instance();

    M_color_list_box->addItem( new ColorItem( tr( "Field" ),
                                              o->fieldBrush().color(),
                                              std::bind( &Options::setFieldColor, o, _1 ) ) );
    M_color_list_box->addItem( new ColorItem( tr( "Line" ),
                                              o->linePen().color(),
                                              std::bind( &Options::setLineColor, o, _1 ) ) );
    M_color_list_box->addItem( new ColorItem( tr( "Score Board Font" ),
                                              o->scoreBoardPen().color(),
                                              std::bind( &Options::setScoreBoardPenColor, o, _1 ) ) );
    M_color_list_box->addItem( new ColorItem( tr( "Score Board Background" ),
                                              o->scoreBoardBrush().color(),
                                              std::bind( &Options::setScoreBoardBrushColor, o, _1 ) ) );

    M_color_list_box->addItem( new ColorItem( tr( "Ball" ),
                                              o->ballPen().color(),
                                              std::bind( &Options::setBallColor, o, _1 ) ) );
    M_color_list_box->addItem( new ColorItem( tr( "Ball Velocity" ),
                                              o->ballVelPen().color(),
                                              std::bind( &Options::setBallVelColor, o, _1 ) ) );

    M_color_list_box->addItem( new ColorItem( tr( "Player Pen" ),
                                              o->playerPen().color(),
                                              std::bind( &Options::setPlayerPenColor, o, _1 ) ) );
    M_color_list_box->addItem( new ColorItem( tr( "Left Team" ),
                                              o->leftTeamPen().color(),
                                              std::bind( &Options::setLeftTeamColor, o, _1 ) ) );
    M_color_list_box->addItem( new ColorItem( tr( "Left Goalie" ),
                                              o->leftGoaliePen().color(),
                                              std::bind( &Options::setLeftGoalieColor, o, _1 ) ) );
    M_color_list_box->addItem( new ColorItem( tr( "Right Team" ),
                                              o->rightTeamPen().color(),
                                              std::bind( &Options::setRightTeamColor, o, _1 ) ) );
    M_color_list_box->addItem( new ColorItem( tr( "Right Goalie" ),
                                              o->rightGoaliePen().color(),
                                              std::bind( &Options::setRightGoalieColor, o, _1 ) ) );
    M_color_list_box->addItem( new ColorItem( tr( "Player Number" ),
                                              o->playerNumberPen().color(),
                                              std::bind( &Options::setPlayerNumberColor, o, _1 ) ) );
//     M_color_list_box->addItem( new ColorItem( tr( "Player Number (Interior)" ),
//                                               o->playerNumberInnerPen().color(),
//                                               std::bind( &Options::setPlayerNumberInnerColor, o, _1 ) ) );
    M_color_list_box->addItem( new ColorItem( tr( "Neck" ),
                                              o->neckPen().color(),
                                              std::bind( &Options::setNeckColor, o, _1 ) ) );
    M_color_list_box->addItem( new ColorItem( tr( "View Area" ),
                                              o->viewAreaPen().color(),
                                              std::bind( &Options::setViewAreaColor, o, _1 ) ) );
    M_color_list_box->addItem( new ColorItem( tr( "Large View Area" ),
                                              o->largeViewAreaPen().color(),
                                              std::bind( &Options::setLargeViewAreaColor, o, _1 ) ) );
    M_color_list_box->addItem( new ColorItem( tr( "Focus Point" ),
                                              o->focusPointPen().color(),
                                              std::bind( &Options::setFocusPointColor, o, _1 ) ) );
    M_color_list_box->addItem( new ColorItem( tr( "Ball Collision" ),
                                              o->ballCollideBrush().color(),
                                              std::bind( &Options::setBallCollideColor, o, _1 ) ) );
    M_color_list_box->addItem( new ColorItem( tr( "Player Collision" ),
                                              o->playerCollideBrush().color(),
                                              std::bind( &Options::setPlayerCollideColor, o, _1 ) ) );
    M_color_list_box->addItem( new ColorItem( tr( "Effort Decayed" ),
                                              o->effortDecayedPen().color(),
                                              std::bind( &Options::setEffortDecayedColor, o, _1 ) ) );
    M_color_list_box->addItem( new ColorItem( tr( "Recovery Decayed" ),
                                              o->recoveryDecayedPen().color(),
                                              std::bind( &Options::setRecoveryDecayedColor, o, _1 ) ) );
    M_color_list_box->addItem( new ColorItem( tr( "Kick" ),
                                              o->kickPen().color(),
                                              std::bind( &Options::setKickColor, o, _1 ) ) );
    M_color_list_box->addItem( new ColorItem( tr( "Kick Fault" ),
                                              o->kickFaultBrush().color(),
                                              std::bind( &Options::setKickFaultColor, o, _1 ) ) );
    M_color_list_box->addItem( new ColorItem( tr( "Kick Acceleration" ),
                                              o->kickAccelPen().color(),
                                              std::bind( &Options::setKickAccelColor, o, _1 ) ) );
    M_color_list_box->addItem( new ColorItem( tr( "Catch" ),
                                              o->catchBrush().color(),
                                              std::bind( &Options::setCatchColor, o, _1 ) ) );
    M_color_list_box->addItem( new ColorItem( tr( "Catch Fault" ),
                                              o->catchFaultBrush().color(),
                                              std::bind( &Options::setCatchFaultColor, o, _1 ) ) );
    M_color_list_box->addItem( new ColorItem( tr( "Tackle" ),
                                              o->tacklePen().color(),
                                              std::bind( &Options::setTackleColor, o, _1 ) ) );
    M_color_list_box->addItem( new ColorItem( tr( "Tackle Fault" ),
                                              o->tackleFaultBrush().color(),
                                              std::bind( &Options::setTackleFaultColor, o, _1 ) ) );
    M_color_list_box->addItem( new ColorItem( tr( "Foul Charged" ),
                                              o->foulChargedBrush().color(),
                                              std::bind( &Options::setFoulChargedColor, o, _1 ) ) );
    M_color_list_box->addItem( new ColorItem( tr( "Pointto" ),
                                              o->pointtoPen().color(),
                                              std::bind( &Options::setPointtoColor, o, _1 ) ) );
    M_color_list_box->addItem( new ColorItem( tr( "Illegal Defense" ),
                                              o->illegalDefensePen().color(),
                                              std::bind( &Options::setIllegalDefenseColor, o, _1 ) ) );
}

/*-------------------------------------------------------------------*/
/*!

*/
QLayout *
ConfigDialog::createFontButtons()
{
    using std::placeholders::_1;

    QVBoxLayout * layout = new QVBoxLayout();
    layout->setContentsMargins( 4, 4, 4, 4 );
    layout->setSpacing( 2 );

    Options * o = &Options::instance();

    {
        FontButton * btn = new FontButton( tr( "Score Board" ),
                                           o->scoreBoardFont(),
                                           std::bind( &Options::setScoreBoardFont, o, _1 ),
                                           this );
        layout->addWidget( btn, 0, Qt::AlignLeft );
    }
    {
        FontButton * btn = new FontButton( tr( "Player" ),
                                           o->playerFont(),
                                           std::bind( &Options::setPlayerFont, o, _1 ),
                                           this );
        layout->addWidget( btn, 0, Qt::AlignLeft );
    }
    {
        FontButton * btn = new FontButton( tr( "Measure" ),
                                           o->measureFont(),
                                           std::bind( &Options::setMeasureFont, o, _1 ),
                                           this );
        layout->addWidget( btn, 0, Qt::AlignLeft );
    }

    layout->addStretch();

    return layout;
}

/*-------------------------------------------------------------------*/
/*
  The 3D tab and the Player Camera tab (docs/PLAN_3D-VIEW.md §5.7, as
  rcssmonitor-web's config-dialog.ts). Every item has a tooltip with the
  details; below a row of choices, a line sums up the chosen one. A number
  field applies with Enter or on leaving it: while what is typed is out of
  its range, it shows the error color, and a value out of the range is
  brought to the nearest end. The numbers that the chosen styles do not use
  are dim.
*/

namespace {

//! a choice of a row of radio buttons: the label, the line below the row, the tooltip
struct ChoiceText {
    const char * label;
    const char * summary;
    const char * tooltip; //!< the summary if null
};

// the choices of the Player Camera tab, in the order of player_camera_options.h
const ChoiceText POSITION_TEXTS[] = {
    { nullptr,
      "At the eyes, 1.65 m high; the player itself is not drawn.",
      "At the eyes, 1.65 m high, looking at the point 10 m ahead and 1 m high.\n"
      "The player itself is not drawn." },
    { nullptr,
      "6 m behind the player and 3 m high.",
      "6 m behind the player and 3 m high, looking at the point 15 m ahead and 1 m high." },
};

const ChoiceText DIRECTION_TEXTS[] = {
    { nullptr,
      "The face direction: body + neck.",
      "The goal is the face direction, body + neck: the center of the view cone." },
    { nullptr,
      "The body direction: not the neck.",
      "The goal is the body direction: the camera does not follow the neck." },
    { nullptr,
      "Halfway: body + neck / 2.",
      "The goal is halfway between the body and the face: body + neck / 2." },
};

const ChoiceText MOTION_TEXTS[] = {
    { nullptr, "The goal direction at every frame.", nullptr },
    { nullptr,
      "Turns by e\u00b7(1 \u2212 exp(\u2212\u0394t / \u03c4)) a frame, at most \u03c9\u00b7\u0394t.",
      "Each frame, the camera turns by e\u00b7(1 \u2212 exp(\u2212\u0394t / \u03c4)), but at most \u03c9\u00b7\u0394t:\n"
      "e is the angle left to the goal and \u0394t the time since the last frame (up to 0.1 s).\n"
      "So it turns at \u03c9 while e is over about \u03c9\u00b7\u03c4, and then slows down.\n"
      "It takes the goal at once when another player is selected, the player moves\n"
      "over 3 m in a frame (e.g. by a move), or the 3D camera or an option here changes." },
};

const ChoiceText FOV_TEXTS[] = {
    { nullptr,
      "The player's view width, within 40\u00b0\u2013120\u00b0.",
      "The player's view width, within 40\u00b0\u2013120\u00b0: the wide 180\u00b0 is shown as 120\u00b0,\n"
      "an unknown width as 90\u00b0." },
    { nullptr,
      "Toward the view width, by (1 \u2212 exp(\u2212\u0394t / \u03c4)) a frame.",
      "Each frame, the field of view moves (1 \u2212 exp(\u2212\u0394t / \u03c4)) of the way to the view width\n"
      "(as View Width; not limited by \u03c9)." },
    { nullptr, "Always Fixed Field of View, whatever the view width.", nullptr },
};

const ChoiceText SIGHT_TEXTS[] = {
    { nullptr,
      "Everything, as the other cameras draw it.",
      "Everything, as the other cameras draw it (the players not connected in black)." },
    { nullptr,
      "Only the players and the ball in the view cone.",
      "Only the players and the ball in the view cone, at any distance:\n"
      "|the angle from the face direction| < the view width / 2." },
    { nullptr,
      "As rcssserver tells the player: the numbers fade out from 20 m to 40 m, "
      "and those within 3 m out of the view are gray.",
      "As rcssserver tells the player. In the cone, the players up to\n"
      "player_max_observation_length (by default the diagonal of the pitch): the number\n"
      "has the opacity 1 \u2212 (d \u2212 unum_far_length) / (unum_too_far_length \u2212 unum_far_length)\n"
      "within 0\u20131, by default 1 under 20 m and 0 from 40 m, and without it the direction\n"
      "marks and the view area are not drawn. The team colors fade to gray in the same way\n"
      "by team_far_length and team_too_far_length. Outside the cone, the players within\n"
      "visible_distance (3 m) are drawn in gray only. The ball is drawn in the cone up to\n"
      "ball_max_observation_length, or within visible_distance. Unlike the server, the true\n"
      "distance is used every cycle, and the chances show as opacity and gray." },
};

// the choices of the compass and the minimap, in the order of guideModes()
const ChoiceText GUIDE_TEXTS[] = {
    { "Off", "Not shown.", nullptr },
    { "Player Camera", "Shown with the Player camera only.", nullptr },
    { "All Cameras", "Shown with every camera but Top, which is a map already.", nullptr },
};

/*!
  \brief the radio buttons' texts of a list of player_camera_options.h
*/
template < typename E, std::size_t N, std::size_t M >
std::vector< std::pair< QString, QString > >
choicesOf( const std::array< PlayerCameraStyle::Info< E >, N > & list,
           const ChoiceText ( & texts )[M] )
{
    static_assert( N == M, "a text for each choice" );
    std::vector< std::pair< QString, QString > > choices;
    for ( std::size_t i = 0; i < N; ++i )
    {
        choices.emplace_back( QString::fromUtf8( texts[i].label ? texts[i].label : list[i].label ),
                              QString::fromUtf8( texts[i].summary ) );
    }
    return choices;
}

//! the tooltips of the choices, by the radio buttons' ids
template < std::size_t M >
void
setChoiceTooltips( QButtonGroup * group,
                   const ChoiceText ( & texts )[M] )
{
    for ( std::size_t i = 0; i < M; ++i )
    {
        if ( QAbstractButton * b = group->button( int( i ) ) )
        {
            b->setToolTip( QString::fromUtf8( texts[i].tooltip ? texts[i].tooltip : texts[i].summary ) );
        }
    }
}

//! the index of a value in a list of player_camera_options.h
template < typename E, std::size_t N >
int
indexOf( const std::array< PlayerCameraStyle::Info< E >, N > & list,
         const E value )
{
    for ( std::size_t i = 0; i < N; ++i )
    {
        if ( list[i].value == value ) return int( i );
    }
    return 0;
}

int
guideIndex( const GuideMode mode )
{
    for ( std::size_t i = 0; i < guideModes().size(); ++i )
    {
        if ( guideModes()[i].first == mode ) return int( i );
    }
    return 0;
}

QString
guideLabel( const GuideMode mode )
{
    return QString::fromUtf8( GUIDE_TEXTS[guideIndex( mode )].label );
}

//! the width of the notes, in average characters
constexpr int NOTE_CHARS = 60;

//! a line of notes below a group or a row of choices, wrapped at a fixed width
QLabel *
noteLabel( const QString & text )
{
    QLabel * label = new QLabel( text );
    label->setWordWrap( true );
    label->setFixedWidth( label->fontMetrics().averageCharWidth() * NOTE_CHARS );
    return label;
}

//! the error color of a number field while what is typed is out of its range
void
markNumber( QDoubleSpinBox * spin )
{
    QPalette palette = QApplication::palette( spin );
    if ( ! spin->hasAcceptableInput() )
    {
        const QColor base = palette.color( QPalette::Base );
        const QColor error( 255, 64, 64 );
        palette.setColor( QPalette::Base,
                          QColor( ( base.red() * 6 + error.red() * 4 ) / 10,
                                  ( base.green() * 6 + error.green() * 4 ) / 10,
                                  ( base.blue() * 6 + error.blue() * 4 ) / 10 ) );
    }
    spin->setPalette( palette );
}

QString
onOff( const bool on )
{
    return on ? QStringLiteral( "On" ) : QStringLiteral( "Off" );
}

}

/*-------------------------------------------------------------------*/
/*!
  a row of radio buttons after its label (ids 0, 1, ... in the order of
  choices), and if note is given, a line below the row that a slot fills
  with the chosen one's summary (the buttons' "summary" property)
*/
QButtonGroup *
ConfigDialog::addChoices( QGridLayout * layout,
                          const QString & label,
                          const QString & tooltip,
                          const std::vector< std::pair< QString, QString > > & choices,
                          QLabel ** note )
{
    const int row = layout->rowCount();

    if ( ! label.isEmpty() )
    {
        QLabel * l = new QLabel( label );
        l->setToolTip( tooltip );
        layout->addWidget( l, row, 0 );
    }

    QButtonGroup * group = new QButtonGroup( this );
    QHBoxLayout * buttons = new QHBoxLayout();
    buttons->setContentsMargins( 0, 0, 0, 0 );
    buttons->setSpacing( 8 );
    for ( std::size_t i = 0; i < choices.size(); ++i )
    {
        QRadioButton * b = new QRadioButton( choices[i].first );
        b->setProperty( "summary", choices[i].second );
        group->addButton( b, int( i ) );
        buttons->addWidget( b );
    }
    buttons->addStretch();
    layout->addLayout( buttons, row, 1, 1, 3 );
    layout->setColumnStretch( 1, 1 ); // the labels as narrow as they can be

    if ( note )
    {
        // as high as the longest summary, so that the dialog keeps its size
        QLabel * n = noteLabel( QString() );
        n->setContentsMargins( 4, 0, 0, 4 );
        const int width = n->width();
        int height = 0;
        for ( const auto & c : choices )
        {
            height = std::max( height,
                               n->fontMetrics().boundingRect( QRect( 0, 0, width - 4, 10000 ),
                                                              Qt::TextWordWrap,
                                                              c.second ).height() );
        }
        n->setMinimumHeight( height + 4 );
        n->setAlignment( Qt::AlignLeft | Qt::AlignTop );
        layout->addWidget( n, row + 1, 1, 1, 3 );
        *note = n;
    }

    return group;
}

/*-------------------------------------------------------------------*/
/*!
  a number field after its label at (row, column) and (row, column + 1).
  The tooltip tells the range in unit.
*/
QDoubleSpinBox *
ConfigDialog::addNumber( QGridLayout * layout,
                         const int row,
                         const int column,
                         const QString & label,
                         const QString & tooltip,
                         const double min,
                         const double max,
                         const double step,
                         const int decimals,
                         const QString & unit,
                         QLabel ** label_widget )
{
    const QString details = tooltip
        + tr( "\nRange: %1\u2013%2%3; a value outside it is brought to the nearest end." )
        .arg( min ).arg( max ).arg( unit );

    QLabel * l = new QLabel( label );
    l->setToolTip( details );
    layout->addWidget( l, row, column );
    if ( label_widget ) *label_widget = l;

    QDoubleSpinBox * spin = new QDoubleSpinBox();
    spin->setRange( min, max );
    spin->setSingleStep( step );
    spin->setDecimals( decimals );
    spin->setKeyboardTracking( false ); // apply with Enter or on leaving the field
    spin->setCorrectionMode( QAbstractSpinBox::CorrectToNearestValue );
    spin->setToolTip( details );
    l->setBuddy( spin );
    // what is typed: QDoubleSpinBox::textChanged() waits for Enter without
    // the keyboard tracking
    connect( spin->findChild< QLineEdit * >(), &QLineEdit::textChanged,
             this, [spin]( const QString & ) { markNumber( spin ); } );
    layout->addWidget( spin, row, column + 1 );
    return spin;
}

/*-------------------------------------------------------------------*/
/*!
  the close-up cameras, and for every camera of the 3D view, the compass,
  the minimap, the boards and the banners
*/
QWidget *
ConfigDialog::create3DTab()
{
    QFrame * frame = new QFrame();
    QVBoxLayout * top_layout = new QVBoxLayout();
    top_layout->setSizeConstraint( QLayout::SetFixedSize );
    top_layout->setContentsMargins( 4, 4, 4, 4 );
    top_layout->setSpacing( 4 );

    // close-up camera
    {
        QGroupBox * group_box = new QGroupBox( tr( "Close-up Camera" ) );
        QVBoxLayout * box = new QVBoxLayout();
        QGridLayout * layout = new QGridLayout();
        layout->setContentsMargins( 0, 0, 0, 0 );

        M_closeup_distance_3d
            = addNumber( layout, 0, 0, tr( "Distance (m):" ),
                         tr( "How far the close-up cameras are from the ball or the player they follow,\n"
                             "in meters. Z and X (and Ctrl+Z) over the 3D view change it too." ),
                         Options::MIN_CLOSEUP_DISTANCE_3D, Options::MAX_CLOSEUP_DISTANCE_3D,
                         0.5, 1, tr( " m" ) );
        M_closeup_distance_3d->setObjectName( "closeup_distance_3d" );
        connect( M_closeup_distance_3d, QOverload< double >::of( &QDoubleSpinBox::valueChanged ),
                 this, [this]( double value ) {
                     Options::instance().setCloseupDistance3D( value );
                     changed3D();
                 } );

        M_closeup_elevation_3d
            = addNumber( layout, 0, 3, tr( "Elevation (\u00b0):" ),
                         tr( "How high the close-up cameras look down from, in degrees above the ground\n"
                             "(the broadcast camera: 32\u00b0)." ),
                         Options::MIN_CLOSEUP_ELEVATION_3D, Options::MAX_CLOSEUP_ELEVATION_3D,
                         1.0, 0, tr( "\u00b0" ) );
        M_closeup_elevation_3d->setObjectName( "closeup_elevation_3d" );
        connect( M_closeup_elevation_3d, QOverload< double >::of( &QDoubleSpinBox::valueChanged ),
                 this, [this]( double value ) {
                     Options::instance().setCloseupElevation3D( value );
                     changed3D();
                 } );
        layout->setColumnMinimumWidth( 2, 16 );
        layout->setColumnStretch( 5, 1 );

        box->addLayout( layout );
        box->addWidget( noteLabel( tr( "Close-up: Ball and Close-up: Selected Player (View > 3D Camera, E) "
                                       "follow from the near touch line as the broadcast camera does, but nearer; "
                                       "without a selected player, the ball." ) ) );
        group_box->setLayout( box );
        top_layout->addWidget( group_box );
    }

    // guides
    {
        QGroupBox * group_box = new QGroupBox( tr( "Guides" ) );
        QVBoxLayout * box = new QVBoxLayout();
        QGridLayout * layout = new QGridLayout();
        layout->setContentsMargins( 0, 0, 0, 0 );

        std::vector< std::pair< QString, QString > > choices;
        for ( const ChoiceText & t : GUIDE_TEXTS )
        {
            choices.emplace_back( QString::fromUtf8( t.label ), QString::fromUtf8( t.summary ) );
        }

        M_compass_3d = addChoices( layout, tr( "Compass:" ),
                                   tr( "A strip at the top: the 360\u00b0 around the camera direction (the triangle),\n"
                                       "with the directions of the goals (L, R in the team colors) and the ball, and\n"
                                       "what the camera sees brighter. Clockwise on the field is to the right." ),
                                   choices, nullptr );
        setChoiceTooltips( M_compass_3d, GUIDE_TEXTS );
        M_compass_3d->setObjectName( "compass_3d" );
        connect( M_compass_3d, &QButtonGroup::idClicked,
                 this, [this]( int id ) {
                     Options::instance().setCompass3D( guideModes()[id].first );
                     changed3D();
                 } );

        M_minimap_3d = addChoices( layout, tr( "Minimap:" ),
                                   tr( "The pitch at the bottom right as the 2D view shows it, with the players,\n"
                                       "the ball, and where the camera is and what it sees." ),
                                   choices, nullptr );
        setChoiceTooltips( M_minimap_3d, GUIDE_TEXTS );
        M_minimap_3d->setObjectName( "minimap_3d" );
        connect( M_minimap_3d, &QButtonGroup::idClicked,
                 this, [this]( int id ) {
                     Options::instance().setMinimap3D( guideModes()[id].first );
                     changed3D();
                 } );

        box->addLayout( layout );
        box->addWidget( noteLabel( tr( "Which way the camera looks; they show everything, "
                                       "whatever the Player Sight." ) ) );
        group_box->setLayout( box );
        top_layout->addWidget( group_box );
    }

    // stadium
    {
        QGroupBox * group_box = new QGroupBox( tr( "Stadium" ) );
        QVBoxLayout * box = new QVBoxLayout();
        QHBoxLayout * layout = new QHBoxLayout();
        layout->setContentsMargins( 0, 0, 0, 0 );

        M_boards_3d_cb = new QCheckBox( tr( "Boards" ) );
        M_boards_3d_cb->setObjectName( "boards_3d" );
        M_boards_3d_cb->setToolTip( tr( "Boards around the pitch, 5.5 m out and 1 m high: a dark shade of the team\n"
                                        "color with the team name and edges in the team color, changing at the\n"
                                        "halfway line." ) );
        connect( M_boards_3d_cb, &QCheckBox::clicked,
                 this, [this]( bool on ) {
                     Options::instance().setBoards3D( on );
                     changed3D();
                 } );
        layout->addWidget( M_boards_3d_cb );

        M_banners_3d_cb = new QCheckBox( tr( "Banners" ) );
        M_banners_3d_cb->setObjectName( "banners_3d" );
        M_banners_3d_cb->setToolTip( tr( "A banner behind each goal, 22 m back and floating 12\u201320 m high, with the\n"
                                         "team name and logo (with Team Graphic): above the goal, so that the goal\n"
                                         "stands out against the ground beyond." ) );
        connect( M_banners_3d_cb, &QCheckBox::clicked,
                 this, [this]( bool on ) {
                     Options::instance().setBanners3D( on );
                     changed3D();
                 } );
        layout->addWidget( M_banners_3d_cb );
        layout->addStretch();

        box->addLayout( layout );
        box->addWidget( noteLabel( tr( "The boards and the banners are in the team colors, to tell the ends apart; "
                                       "the goal nets always are." ) ) );
        group_box->setLayout( box );
        top_layout->addWidget( group_box );
    }

    // default
    {
        QPushButton * button = new QPushButton( tr( "Default" ) );
        button->setObjectName( "default_3d" );
        button->setToolTip( tr( "Set the defaults: Close-up %1 m, %2\u00b0, Compass %3, Minimap %4,\n"
                                "Boards %5, Banners %6" )
                            .arg( Options::CLOSEUP_DISTANCE_3D )
                            .arg( Options::CLOSEUP_ELEVATION_3D )
                            .arg( guideLabel( Options::COMPASS_3D ) )
                            .arg( guideLabel( Options::MINIMAP_3D ) )
                            .arg( onOff( Options::BOARDS_3D ) )
                            .arg( onOff( Options::BANNERS_3D ) ) );
        button->setAutoDefault( false );
        connect( button, &QPushButton::clicked,
                 this, [this]() {
                     Options::instance().setView3DDefaults();
                     changed3D();
                 } );
        top_layout->addWidget( button, 0, Qt::AlignLeft );
    }

    top_layout->addStretch();
    frame->setLayout( top_layout );
    return frame;
}

/*-------------------------------------------------------------------*/
/*!
  the selected player's camera of the 3D view, in the styles to compare,
  and what it draws
*/
QWidget *
ConfigDialog::createPlayerCameraTab()
{
    using namespace PlayerCameraStyle;
    QFrame * frame = new QFrame();
    QVBoxLayout * top_layout = new QVBoxLayout();
    top_layout->setSizeConstraint( QLayout::SetFixedSize );
    top_layout->setContentsMargins( 4, 4, 4, 4 );
    top_layout->setSpacing( 4 );

    top_layout->addWidget( noteLabel( tr( "The 3D view's Player camera, at the selected player. "
                                          "Point at an item for details." ) ) );

    // a style changed by the radio buttons: a copy with one style changed
    const auto set_style = [this]( const std::function< void ( PlayerCameraOptions & ) > & change ) {
        PlayerCameraOptions o = Options::instance().playerCamera();
        change( o );
        Options::instance().setPlayerCamera( o );
        changed3D();
    };

    // camera
    {
        QGroupBox * group_box = new QGroupBox( tr( "Camera" ) );
        QGridLayout * layout = new QGridLayout();
        layout->setVerticalSpacing( 0 );

        M_player_camera_position
            = addChoices( layout, tr( "Position:" ),
                          tr( "Where the camera is, along the camera direction" ),
                          choicesOf( positions(), POSITION_TEXTS ), &M_player_camera_position_note );
        setChoiceTooltips( M_player_camera_position, POSITION_TEXTS );
        M_player_camera_position->setObjectName( "player_camera_position" );
        connect( M_player_camera_position, &QButtonGroup::idClicked,
                 this, [set_style]( int id ) {
                     set_style( [id]( PlayerCameraOptions & o ) { o.position = positions()[id].value; } );
                 } );

        M_player_camera_direction
            = addChoices( layout, tr( "Direction:" ),
                          tr( "The goal direction that the camera turns to" ),
                          choicesOf( directions(), DIRECTION_TEXTS ), &M_player_camera_direction_note );
        setChoiceTooltips( M_player_camera_direction, DIRECTION_TEXTS );
        M_player_camera_direction->setObjectName( "player_camera_direction" );
        connect( M_player_camera_direction, &QButtonGroup::idClicked,
                 this, [set_style]( int id ) {
                     set_style( [id]( PlayerCameraOptions & o ) { o.direction = directions()[id].value; } );
                 } );

        M_player_camera_motion
            = addChoices( layout, tr( "Motion:" ),
                          tr( "How the camera turns to the goal direction" ),
                          choicesOf( motions(), MOTION_TEXTS ), &M_player_camera_motion_note );
        setChoiceTooltips( M_player_camera_motion, MOTION_TEXTS );
        M_player_camera_motion->setObjectName( "player_camera_motion" );
        connect( M_player_camera_motion, &QButtonGroup::idClicked,
                 this, [set_style]( int id ) {
                     set_style( [id]( PlayerCameraOptions & o ) { o.motion = motions()[id].value; } );
                 } );

        M_player_camera_fov
            = addChoices( layout, tr( "Field of View:" ),
                          tr( "The horizontal field of view h. The vertical one follows the shape of the view:\n"
                              "2\u00b7atan(tan(h / 2) / (width / height))." ),
                          choicesOf( fovs(), FOV_TEXTS ), &M_player_camera_fov_note );
        setChoiceTooltips( M_player_camera_fov, FOV_TEXTS );
        M_player_camera_fov->setObjectName( "player_camera_fov" );
        connect( M_player_camera_fov, &QButtonGroup::idClicked,
                 this, [set_style]( int id ) {
                     set_style( [id]( PlayerCameraOptions & o ) { o.fov = fovs()[id].value; } );
                 } );

        // the numbers: below the choices, in 2 columns of a label and a field
        QGridLayout * numbers = new QGridLayout();
        numbers->setContentsMargins( 0, 4, 0, 0 );

        M_player_camera_time_constant
            = addNumber( numbers, 0, 0, tr( "Time Constant \u03c4 (s):" ),
                         tr( "\u03c4 of Smooth and Eased: after \u03c4 seconds, about 37% (1/e) of the difference\n"
                             "is left (while Smooth turns slower than \u03c9). With 0, Smooth turns at \u03c9 only\n"
                             "and Eased follows at once." ),
                         MIN_TIME_CONSTANT, MAX_TIME_CONSTANT, 0.05, 2, tr( " s" ),
                         &M_player_camera_time_constant_label );
        M_player_camera_time_constant->setObjectName( "player_camera_time_constant" );
        connect( M_player_camera_time_constant, QOverload< double >::of( &QDoubleSpinBox::valueChanged ),
                 this, [set_style]( double value ) {
                     set_style( [value]( PlayerCameraOptions & o ) { o.time_constant = value; } );
                 } );

        M_player_camera_max_turn
            = addNumber( numbers, 0, 3, tr( "Max Turn \u03c9 (\u00b0/s):" ),
                         tr( "\u03c9 of Smooth: the fastest the camera turns, in degrees per second." ),
                         MIN_MAX_TURN, MAX_MAX_TURN, 1.0, 0, tr( "\u00b0/s" ),
                         &M_player_camera_max_turn_label );
        M_player_camera_max_turn->setObjectName( "player_camera_max_turn" );
        connect( M_player_camera_max_turn, QOverload< double >::of( &QDoubleSpinBox::valueChanged ),
                 this, [set_style]( double value ) {
                     set_style( [value]( PlayerCameraOptions & o ) { o.max_turn = value; } );
                 } );

        M_player_camera_fixed_fov
            = addNumber( numbers, 1, 0, tr( "Fixed Field of View (\u00b0):" ),
                         tr( "The horizontal field of view of Fixed, in degrees." ),
                         MIN_FIXED_FOV, MAX_FIXED_FOV, 1.0, 0, tr( "\u00b0" ),
                         &M_player_camera_fixed_fov_label );
        M_player_camera_fixed_fov->setObjectName( "player_camera_fixed_fov" );
        connect( M_player_camera_fixed_fov, QOverload< double >::of( &QDoubleSpinBox::valueChanged ),
                 this, [set_style]( double value ) {
                     set_style( [value]( PlayerCameraOptions & o ) { o.fixed_fov = value; } );
                 } );
        numbers->setColumnMinimumWidth( 2, 16 );
        numbers->setColumnStretch( 5, 1 );

        layout->addLayout( numbers, layout->rowCount(), 0, 1, 4 );
        group_box->setLayout( layout );
        top_layout->addWidget( group_box );
    }

    // player sight
    {
        QGroupBox * group_box = new QGroupBox( tr( "Player Sight" ) );
        QVBoxLayout * box = new QVBoxLayout();
        QGridLayout * layout = new QGridLayout();
        layout->setContentsMargins( 0, 0, 0, 0 );
        layout->setVerticalSpacing( 0 );

        M_player_camera_sight
            = addChoices( layout, QString(), QString(),
                          choicesOf( sights(), SIGHT_TEXTS ), &M_player_camera_sight_note );
        setChoiceTooltips( M_player_camera_sight, SIGHT_TEXTS );
        M_player_camera_sight->setObjectName( "player_camera_sight" );
        connect( M_player_camera_sight, &QButtonGroup::idClicked,
                 this, [set_style]( int id ) {
                     set_style( [id]( PlayerCameraOptions & o ) { o.sight = sights()[id].value; } );
                 } );

        box->addLayout( layout );
        box->addWidget( noteLabel( tr( "Only with a connected player selected, by its face direction "
                                       "and view width in the log." ) ) );
        group_box->setLayout( box );
        top_layout->addWidget( group_box );
    }

    // default
    {
        const PlayerCameraOptions d;
        QPushButton * button = new QPushButton( tr( "Default" ) );
        button->setObjectName( "default_player_camera" );
        button->setToolTip( tr( "Set the default player camera: %1, %2, %3, %4, %5,\n"
                                "\u03c4 %6 s, \u03c9 %7\u00b0/s, Fixed %8\u00b0" )
                            .arg( QString::fromUtf8( positions()[indexOf( positions(), d.position )].label ) )
                            .arg( QString::fromUtf8( directions()[indexOf( directions(), d.direction )].label ) )
                            .arg( QString::fromUtf8( motions()[indexOf( motions(), d.motion )].label ) )
                            .arg( QString::fromUtf8( fovs()[indexOf( fovs(), d.fov )].label ) )
                            .arg( QString::fromUtf8( sights()[indexOf( sights(), d.sight )].label ) )
                            .arg( d.time_constant )
                            .arg( d.max_turn )
                            .arg( d.fixed_fov ) );
        button->setAutoDefault( false );
        connect( button, &QPushButton::clicked,
                 this, [this]() {
                     Options::instance().setPlayerCamera( PlayerCameraOptions() );
                     changed3D();
                 } );
        top_layout->addWidget( button, 0, Qt::AlignLeft );
    }

    top_layout->addStretch();
    frame->setLayout( top_layout );
    return frame;
}

/*-------------------------------------------------------------------*/
/*!
  a setting of the 3D tab or the Player Camera tab changed: the tabs show
  it, and the views draw it
*/
void
ConfigDialog::changed3D()
{
    update3D();
    emit configured();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::update3D()
{
    if ( ! M_closeup_distance_3d )
    {
        return; // a build without the 3D view
    }

    using namespace PlayerCameraStyle;
    const Options & opt = Options::instance();
    const PlayerCameraOptions & pc = opt.playerCamera();

    // without valueChanged(), which would write the shown (rounded) value back
    const auto set_number = []( QDoubleSpinBox * spin, const double value ) {
        const QSignalBlocker blocker( spin );
        spin->setValue( value );
        markNumber( spin );
    };
    // the summary of the chosen one below the row
    const auto set_choice = []( QButtonGroup * group, const int id, QLabel * note ) {
        if ( QAbstractButton * b = group->button( id ) )
        {
            b->setChecked( true );
            if ( note ) note->setText( b->property( "summary" ).toString() );
        }
    };

    set_number( M_closeup_distance_3d, opt.closeupDistance3D() );
    set_number( M_closeup_elevation_3d, opt.closeupElevation3D() );
    set_choice( M_compass_3d, guideIndex( opt.compass3D() ), nullptr );
    set_choice( M_minimap_3d, guideIndex( opt.minimap3D() ), nullptr );
    M_boards_3d_cb->setChecked( opt.boards3D() );
    M_banners_3d_cb->setChecked( opt.banners3D() );

    set_choice( M_player_camera_position, indexOf( positions(), pc.position ), M_player_camera_position_note );
    set_choice( M_player_camera_direction, indexOf( directions(), pc.direction ), M_player_camera_direction_note );
    set_choice( M_player_camera_motion, indexOf( motions(), pc.motion ), M_player_camera_motion_note );
    set_choice( M_player_camera_fov, indexOf( fovs(), pc.fov ), M_player_camera_fov_note );
    set_choice( M_player_camera_sight, indexOf( sights(), pc.sight ), M_player_camera_sight_note );
    set_number( M_player_camera_time_constant, pc.time_constant );
    set_number( M_player_camera_max_turn, pc.max_turn );
    set_number( M_player_camera_fixed_fov, pc.fixed_fov );

    // the numbers that the chosen styles do not use are dim
    const bool smooth = ( pc.motion == Motion::Smooth );
    const bool time_constant = ( smooth || pc.fov == Fov::Eased );
    const bool fixed = ( pc.fov == Fov::Fixed );
    M_player_camera_time_constant->setEnabled( time_constant );
    M_player_camera_time_constant_label->setEnabled( time_constant );
    M_player_camera_max_turn->setEnabled( smooth );
    M_player_camera_max_turn_label->setEnabled( smooth );
    M_player_camera_fixed_fov->setEnabled( fixed );
    M_player_camera_fixed_fov_label->setEnabled( fixed );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::showEvent( QShowEvent * event )
{
    updateAll();
    event->accept();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::updateAll()
{
    const Options & opt = Options::instance();

    M_ball_size_text->setText( QString::number( opt.ballSize() ) );
    M_player_size_text->setText( QString::number( opt.playerSize() ) );

    updateFieldScale();

    M_canvas_width_text->setText( QString::number( opt.canvasWidth() ) );
    M_canvas_height_text->setText( QString::number( opt.canvasHeight() ) );

    M_team_graphic_scale->setValue( opt.teamGraphicScale() );

    M_anti_aliasing_cb->setChecked( opt.antiAliasing() );

    M_player_number_cb->setChecked( opt.showPlayerNumber() );
    M_player_type_cb->setChecked( opt.showPlayerType() );
    M_stamina_cb->setChecked( opt.showStamina() );
    M_view_area_cb->setChecked( opt.showViewArea() );
    M_focus_point_cb->setChecked( opt.showFocusPoint() );
    M_all_focus_points_cb->setChecked( opt.showAllFocusPoints() );
    M_all_focus_points_cb->setEnabled( opt.showFocusPoint() );
    M_catch_area_cb->setChecked( opt.showCatchArea() );
    M_tackle_area_cb->setChecked( opt.showTackleArea() );
    M_kick_accel_area_cb->setChecked( opt.showKickAccelArea() );
    M_pointto_cb->setChecked( opt.showPointto() );
    M_card_cb->setChecked( opt.showCard() );
    M_illegal_defense_cb->setChecked( opt.showIllegalDefense() );

    M_show_score_board_cb->setChecked( opt.showScoreBoard() );
    M_show_keepaway_area_cb->setChecked( opt.showKeepawayArea() );
    M_show_team_graphic_cb->setChecked( opt.showTeamGraphic() );
    M_show_draw_info_cb->setChecked( opt.showDrawInfo() );

    M_show_ball_cb->setChecked( opt.showBall() );
    M_show_player_cb->setChecked( opt.showPlayer() );
    M_show_flag_cb->setChecked( opt.showFlag() );
    M_show_offside_line_cb->setChecked( opt.showOffsideLine() );

    switch( opt.focusType() ) {
    case Options::FOCUS_BALL:
        M_focus_ball_rb->setChecked( true );
        break;
    case Options::FOCUS_PLAYER:
        M_focus_player_rb->setChecked( true );
        break;
    case Options::FOCUS_POINT:
        M_focus_fix_rb->setChecked( true );
        break;
    default:
        M_focus_fix_rb->setChecked( true );
        break;
    }

    switch ( opt.playerSelectType() ) {
    case Options::SELECT_AUTO_ALL:
        M_select_all_rb->setChecked( true );
        break;
    case Options::SELECT_AUTO_LEFT:
        M_select_left_rb->setChecked( true );
        break;
    case Options::SELECT_AUTO_RIGHT:
        M_select_right_rb->setChecked( true );
        break;
    case Options::UNSELECT:
        M_unselect_rb->setChecked( true );
        break;
    default:
        M_unselect_rb->setChecked( true );
        break;
    }

//     M_ball_trace_start->setText( QString::number( opt.ballTraceStart() ) );
//     M_ball_trace_end->setText( QString::number( opt.ballTraceEnd() ) );

//     M_player_trace_start->setText( QString::number( opt.playerTraceStart() ) );
//     M_player_trace_end->setText( QString::number( opt.playerTraceEnd() ) );

//     M_ball_vel_cycle->setValue( opt.ballVelCycle() );

    for ( int i = 0; i < M_color_list_box->count(); ++i )
    {
        ColorItem * color_item = dynamic_cast< ColorItem * >( M_color_list_box->item( i ) );
        if ( ! color_item )
        {
            continue;
        }

        color_item->updateColor();
    }

    update3D();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::updateFieldScale()
{
    int iscale = static_cast< int >( rint( Options::instance().fieldScale() * 10.0 ) );
    if ( M_scale_slider->value() != iscale )
    {
        M_scale_slider->setValue( iscale );
    }

    char buf[16];
    snprintf( buf, 16, "%.2f", Options::instance().fieldScale() );
    M_scale_text->setText( QString::fromLatin1( buf ) );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::zoomIn()
{
    Options::instance().zoomIn();

    updateFieldScale();

    emit configured();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::zoomOut()
{
    Options::instance().zoomOut();

    updateFieldScale();

    emit configured();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::fitToScreen()
{
    Options::instance().fitToScreen();
    updateFieldScale();
    M_focus_fix_rb->setChecked( true );

    emit configured();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::editBallSize( const QString & text )
{
    bool ok = true;
    double value = text.toDouble( &ok );

    if ( ok && value > 0.0 )
    {
        Options::instance().setBallSize( value );

        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::editPlayerSize( const QString & text )
{
    bool ok = true;
    double value = text.toDouble( &ok );

    if ( ok )
    {
        Options::instance().setPlayerSize( value );

        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::editFocusPointSize( const QString & text )
{
    bool ok = true;
    double value = text.toDouble( &ok );

    if ( ok )
    {
        Options::instance().setFocusPointSize( value );

        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::slideFieldScale( int value )
{
    double scale = value * 0.1;

    if ( std::fabs( scale - Options::instance().fieldScale() ) >= 0.01 )
    {
        M_scale_text->setText( QString::number( scale ) );
        Options::instance().setFieldScale( scale );

        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::editFieldScale( const QString & text )
{
    bool ok = true;
    double value = text.toDouble( &ok );

    if ( ok
         && std::fabs( value - Options::instance().fieldScale() ) >= 0.01 )
    {
        int ivalue = static_cast< int >( rint( value * 10.0 ) );
        M_scale_slider->setValue( ivalue );
        Options::instance().setFieldScale( value );

        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::applyCanvasSize()
{
    bool ok_w = true;
    bool ok_h = true;
    int width = M_canvas_width_text->text().toInt( & ok_w );
    int height = M_canvas_height_text->text().toInt( & ok_h );

    if ( ok_w
         && ok_h
         && width > 0
         && height > 0 )
    {
        emit canvasResized( QSize( width, height ) );
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::slotTeamGraphicScaleChanged( double value )
{
    if ( std::fabs( value - Options::instance().teamGraphicScale() ) > 0.01 )
    {
        Options::instance().setTeamGraphicScale( value );

        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickAntiAliasing( bool checked )
{
    if ( Options::instance().antiAliasing() != checked )
    {
        Options::instance().toggleAntiAliasing();

        emit configured();
    }
}


/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickShowScoreBoard( bool checked )
{
    if ( Options::instance().showScoreBoard() != checked )
    {
        Options::instance().toggleShowScoreBoard();
        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::toggleShowScoreBoard()
{
    Options::instance().toggleShowScoreBoard();
    M_show_score_board_cb->setChecked( Options::instance().showScoreBoard() );

    emit configured();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickShowKeepawayArea( bool checked )
{
    if ( Options::instance().showKeepawayArea() != checked )
    {
        Options::instance().toggleShowKeepawayArea();
        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::toggleShowKeepawayArea()
{
    Options::instance().toggleShowKeepawayArea();
    M_show_keepaway_area_cb->setChecked( Options::instance().showKeepawayArea() );

    emit configured();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickShowTeamGraphic( bool checked )
{
    if ( Options::instance().showTeamGraphic() != checked )
    {
        Options::instance().toggleShowTeamGraphic();
        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::toggleShowTeamGraphic()
{
    Options::instance().toggleShowTeamGraphic();
    M_show_team_graphic_cb->setChecked( Options::instance().showTeamGraphic() );

    emit configured();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickShowFlag( bool checked )
{
    if ( Options::instance().showFlag() != checked )
    {
        Options::instance().toggleShowFlag();
        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::toggleShowFlag()
{
    Options::instance().toggleShowFlag();
    M_show_flag_cb->setChecked( Options::instance().showFlag() );

    emit configured();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickShowBall( bool checked )
{
    if ( Options::instance().showBall() != checked )
    {
        Options::instance().toggleShowBall();
        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::toggleShowBall()
{
    Options::instance().toggleShowBall();
    M_show_ball_cb->setChecked( Options::instance().showBall() );

    emit configured();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickShowPlayer( bool checked )
{
    if ( Options::instance().showPlayer() != checked )
    {
        Options::instance().toggleShowPlayer();
        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::toggleShowPlayer()
{
    Options::instance().toggleShowPlayer();
    M_show_player_cb->setChecked( Options::instance().showPlayer() );

    emit configured();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickShowPlayerNumber( bool checked )
{
    if ( Options::instance().showPlayerNumber() != checked )
    {
        Options::instance().toggleShowPlayerNumber();
        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::toggleShowPlayerNumber()
{
    Options::instance().toggleShowPlayerNumber();
    M_player_number_cb->setChecked( Options::instance().showPlayerNumber() );

    emit configured();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickShowPlayerType( bool checked )
{
    if ( Options::instance().showPlayerType() != checked )
    {
        Options::instance().toggleShowPlayerType();
        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::toggleShowPlayerType()
{
    Options::instance().toggleShowPlayerType();
    M_player_type_cb->setChecked( Options::instance().showPlayerType() );

    emit configured();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickShowStamina( bool checked )
{
    if ( Options::instance().showStamina() != checked )
    {
        Options::instance().toggleShowStamina();
        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickShowStaminaCapacity( bool checked )
{
    if ( Options::instance().showStaminaCapacity() != checked )
    {
        Options::instance().toggleShowStaminaCapacity();
        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::toggleShowStamina()
{
//     Options::instance().toggleShowStamina();
//     M_stamina_cb->setChecked( Options::instance().showStamina() );

    const Options & opt = Options::instance();

    if ( ! opt.showStamina()
         && ! opt.showStaminaCapacity() )
    {
        Options::instance().toggleShowStamina();
    }
    else if ( opt.showStamina()
              && ! opt.showStaminaCapacity() )
    {
        Options::instance().toggleShowStamina();
        Options::instance().toggleShowStaminaCapacity();
    }
    else if ( ! opt.showStamina()
              && opt.showStaminaCapacity() )
    {
        Options::instance().toggleShowStamina();
    }
    else
    {
        Options::instance().toggleShowStamina();
        Options::instance().toggleShowStaminaCapacity();
    }

    M_stamina_cb->setChecked( opt.showStamina() );
    M_stamina_capacity_cb->setChecked( opt.showStaminaCapacity() );

    emit configured();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::toggleShowStaminaCapacity()
{
    Options::instance().toggleShowStaminaCapacity();
    M_stamina_capacity_cb->setChecked( Options::instance().showStaminaCapacity() );

    emit configured();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickShowViewArea( bool checked )
{
    if ( Options::instance().showViewArea() != checked )
    {
        Options::instance().toggleShowViewArea();
        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::toggleShowViewArea()
{
    Options::instance().toggleShowViewArea();
    M_view_area_cb->setChecked( Options::instance().showViewArea() );

    emit configured();
}


/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickShowFocusPoint( bool checked )
{
    if ( Options::instance().showFocusPoint() != checked )
    {
        Options::instance().toggleShowFocusPoint();
        M_all_focus_points_cb->setEnabled( checked );
        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
void
ConfigDialog::clickShowAllFocusPoints( bool checked )
{
    if ( Options::instance().showAllFocusPoints() != checked )
    {
        Options::instance().toggleShowAllFocusPoints();
        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::toggleShowFocusPoint()
{
    Options::instance().toggleShowFocusPoint();
    M_focus_point_cb->setChecked( Options::instance().showFocusPoint() );
    M_all_focus_points_cb->setEnabled( Options::instance().showFocusPoint() );

    emit configured();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickShowCatchArea( bool checked )
{
    if ( Options::instance().showCatchArea() != checked )
    {
        Options::instance().toggleShowCatchArea();
        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::toggleShowCatchArea()
{
    Options::instance().toggleShowCatchArea();
    M_catch_area_cb->setChecked( Options::instance().showCatchArea() );

    emit configured();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickShowTackleArea( bool checked )
{
    if ( Options::instance().showTackleArea() != checked )
    {
        Options::instance().toggleShowTackleArea();
        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::toggleShowTackleArea()
{
    Options::instance().toggleShowTackleArea();
    M_tackle_area_cb->setChecked( Options::instance().showTackleArea() );

    emit configured();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickShowKickAccelArea( bool checked )
{
    if ( Options::instance().showKickAccelArea() != checked )
    {
        Options::instance().toggleShowKickAccelArea();
        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::toggleShowKickAccelArea()
{
    Options::instance().toggleShowKickAccelArea();
    M_kick_accel_area_cb->setChecked( Options::instance().showKickAccelArea() );

    emit configured();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickShowPointto( bool checked )
{
    if ( Options::instance().showPointto() != checked )
    {
        Options::instance().toggleShowPointto();
        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::toggleShowPointto()
{
    Options::instance().toggleShowPointto();
    M_pointto_cb->setChecked( Options::instance().showPointto() );

    emit configured();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickShowCard( bool checked )
{
    if ( Options::instance().showCard() != checked )
    {
        Options::instance().toggleShowCard();
        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::toggleShowCard()
{
    Options::instance().toggleShowCard();
    M_card_cb->setChecked( Options::instance().showCard() );

    emit configured();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickShowIllegalDefense( bool checked )
{
    if ( Options::instance().showIllegalDefense() != checked )
    {
        Options::instance().toggleShowIllegalDefense();
        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::toggleShowIllegalDefense()
{
    Options::instance().toggleShowIllegalDefense();
    M_illegal_defense_cb->setChecked( Options::instance().showIllegalDefense() );

    emit configured();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickShowOffsideLine( bool checked )
{
    if ( Options::instance().showOffsideLine() != checked )
    {
        Options::instance().toggleShowOffsideLine();
        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::toggleShowOffsideLine()
{
    Options::instance().toggleShowOffsideLine();
    M_show_offside_line_cb->setChecked( Options::instance().showOffsideLine() );

    emit configured();
}


/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickShowDrawInfo( bool checked )
{
    if ( Options::instance().showDrawInfo() != checked )
    {
        Options::instance().toggleShowDrawInfo();
        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::toggleShowDrawInfo()
{
    Options::instance().toggleShowDrawInfo();
    M_show_draw_info_cb->setChecked( Options::instance().showDrawInfo() );

    emit configured();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickShowGridCoord( bool checked )
{
    if ( Options::instance().showGridCoord() != checked )
    {
        Options::instance().toggleShowGridCoord();
        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::slideGridStep( int value )
{
    M_grid_step_text->setText( QString::number( value * 0.1 ) );

    Options::instance().setGridStep( value * 0.1 );

    emit configured();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::editGridStep( const QString & text )
{
    bool ok = true;
    double value = text.toDouble( & ok );

    if ( ok )
    {
        M_grid_step_slider->setValue( static_cast< int >( rint( value * 10.0 ) ) );
        Options::instance().setGridStep( rint( value / 0.001 ) * 0.001 );

        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickFocusBall()
{
    if ( Options::instance().focusType() != Options::FOCUS_BALL )
    {
        Options::instance().setFocusType( Options::FOCUS_BALL );
        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::toggleFocusBall()
{
    if ( Options::instance().focusType() == Options::FOCUS_BALL )
    {
        setFocusFix();
    }
    else
    {
        Options::instance().setFocusType( Options::FOCUS_BALL );
        M_focus_ball_rb->setChecked( true );

        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickFocusPlayer()
{
    if ( Options::instance().focusType() != Options::FOCUS_PLAYER )
    {
        Options::instance().setFocusType( Options::FOCUS_PLAYER );
        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::toggleFocusPlayer()
{
    if ( Options::instance().focusType() == Options::FOCUS_PLAYER )
    {
        setFocusFix();
    }
    else
    {
        Options::instance().setFocusType( Options::FOCUS_PLAYER );
        M_focus_player_rb->setChecked( true );

        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::setFocusFix()
{
    if ( Options::instance().focusType() != Options::FOCUS_POINT )
    {
        Options::instance().setFocusType( Options::FOCUS_POINT );
        M_focus_fix_rb->setChecked( true );

        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::setFocusPoint( const QPoint & point )
{
    Options::instance().setFocusPoint( point.x(), point.y() );
    M_focus_fix_rb->setChecked( true );

    emit configured();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickSelectAutoAll()
{
    if ( Options::instance().playerSelectType() != Options::SELECT_AUTO_ALL )
    {
        Options::instance().setPlayerSelectType( Options::SELECT_AUTO_ALL );
        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::toggleSelectAutoAll()
{
    if ( Options::instance().playerSelectType() == Options::SELECT_AUTO_ALL )
    {
        setUnselect();
    }
    else
    {
        Options::instance().setPlayerSelectType( Options::SELECT_AUTO_ALL );
        M_select_all_rb->setChecked( true );

        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickSelectAutoLeft()
{
    if ( Options::instance().playerSelectType() != Options::SELECT_AUTO_LEFT )
    {
        Options::instance().setPlayerSelectType( Options::SELECT_AUTO_LEFT );
        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::toggleSelectAutoLeft()
{
    if ( Options::instance().playerSelectType() == Options::SELECT_AUTO_LEFT )
    {
        setUnselect();
    }
    else
    {
        Options::instance().setPlayerSelectType( Options::SELECT_AUTO_LEFT );
        M_select_left_rb->setChecked( true );

        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickSelectAutoRight()
{
    if ( Options::instance().playerSelectType() != Options::SELECT_AUTO_RIGHT )
    {
        Options::instance().setPlayerSelectType( Options::SELECT_AUTO_RIGHT );
        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::toggleSelectAutoRight()
{
    if ( Options::instance().playerSelectType() == Options::SELECT_AUTO_RIGHT )
    {
        setUnselect();
    }
    else
    {
        Options::instance().setPlayerSelectType( Options::SELECT_AUTO_RIGHT );
        M_select_right_rb->setChecked( true );

        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickSelectFix()
{
    if ( Options::instance().playerSelectType() != Options::SELECT_FIX )
    {
        Options::instance().setPlayerSelectType( Options::SELECT_FIX );
        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::toggleSelectFix()
{
    if ( Options::instance().playerSelectType() == Options::SELECT_FIX )
    {
        setUnselect();
    }
    else
    {
        Options::instance().setPlayerSelectType( Options::SELECT_FIX );
        M_select_fix_rb->setChecked( true );

        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::setUnselect()
{
    Options::instance().unselectPlayer();
    M_unselect_rb->setChecked( true );
    M_player_choice->setCurrentIndex( 0 );

    emit configured();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::selectPlayer( int number )
{
    // left side:  number = [1,11]
    // right side:  number = [12,22]

    if ( number <= 0
         || 22 < number )
    {
        setUnselect();
        return;
    }

    Options::instance().setPlayerSelectType( Options::SELECT_FIX );
    if ( number <= 11 )
    {
        Options::instance().setSelectedNumber( rcss::rcg::LEFT, number );
    }
    else
    {
        Options::instance().setSelectedNumber( rcss::rcg::RIGHT, number - 11 );
    }

    M_player_choice->setCurrentIndex( number );
    M_select_fix_rb->setChecked( true );

    emit configured();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::choicePlayer( int number )
{
    if ( number <= 11
         && Options::instance().selectedPlayer( rcss::rcg::LEFT, number ) )
    {
        return;
    }

    if ( number > 11
         && Options::instance().selectedPlayer( rcss::rcg::RIGHT, number - 11 ) )
    {
        return;
    }

    selectPlayer( number );
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::selectPlayerWithKey()
{
    // the action has the number of the player as its data: 1-11 for the
    // left players, and 12-22 for the right players
    QAction * action = qobject_cast< QAction * >( sender() );

    if ( ! action )
    {
        return;
    }

    const int number = action->data().toInt();
    if ( 1 <= number && number <= 22 )
    {
        selectPlayer( number );
    }
}

#if 0
/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::editBallTraceStart( const QString & text )
{
    bool ok = true;
    long value = text.toLong( & ok );

    if ( ok )
    {
        Options::instance().setBallTraceStart( value );

        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::editBallTraceEnd( const QString & text )
{
    bool ok = true;
    long value = text.toLong( & ok );

    if ( ok )
    {
        Options::instance().setBallTraceEnd( value );

        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickBallTraceAll()
{
    const std::vector< DispPtr > & cont = M_main_data.dispHolder().dispInfoCont();

    if ( ! cont.empty() )
    {
        if ( Options::instance().ballTraceStart() == cont.front()->show_.time_
             && Options::instance().ballTraceEnd() == cont.back()->show_.time_ )
        {
            M_ball_trace_start->setText( QString::number( 0 ) );
            M_ball_trace_end->setText( QString::number( 0 ) );

            Options::instance().setBallTraceStart( 0 );
            Options::instance().setBallTraceEnd( 0 );
        }
        else
        {
            M_ball_trace_start->setText( QString::number( cont.front()->show_.time_ ) );
            M_ball_trace_end->setText( QString::number( cont.back()->show_.time_ ) );

            Options::instance().setBallTraceStart( cont.front()->show_.time_ );
            Options::instance().setBallTraceEnd( cont.back()->show_.time_ );
        }

        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::editPlayerTraceStart( const QString & text )
{
    bool ok = true;
    long value = text.toLong( & ok );

    if ( ok )
    {
        Options::instance().setPlayerTraceStart( value );

        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::editPlayerTraceEnd( const QString & text )
{
    bool ok = true;
    long value = text.toLong( & ok );

    if ( ok )
    {
        Options::instance().setPlayerTraceEnd( value );

        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickPlayerTraceAll()
{
    const std::vector< DispPtr > & cont = M_main_data.dispHolder().dispInfoCont();
    if ( ! cont.empty() )
    {
        if ( Options::instance().playerTraceStart() == cont.front()->show_.time_
             && Options::instance().playerTraceEnd() == cont.back()->show_.time_ )
        {
            M_player_trace_start->setText( QString::number( 0 ) );
            M_player_trace_end->setText( QString::number( 0 ) );

            Options::instance().setPlayerTraceStart( 0 );
            Options::instance().setPlayerTraceEnd( 0 );
        }
        else
        {
            M_player_trace_start->setText( QString::number( cont.front()->show_.time_ ) );
            M_player_trace_end->setText( QString::number( cont.back()->show_.time_ ) );

            Options::instance().setPlayerTraceStart( cont.front()->show_.time_ );
            Options::instance().setPlayerTraceEnd( cont.back()->show_.time_ );
        }

        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::clickLinePointButton()
{
    Options::instance().toggleLineTrace();

    emit configured();
}
#endif

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::changeBallVelCycle( int value )
{
    Options::instance().setBallVelCycle( value );

    emit configured();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::selectColorEntry( QListWidgetItem * item )
{
    ColorItem * color_item = dynamic_cast< ColorItem * >( item );
    if ( ! color_item )
    {
        return;
    }

    QColor col = QColorDialog::getColor( color_item->newColor() );
    if ( ! col.isValid() )
    {
        return;
    }

    if ( color_item->setColor( col ) )
    {
        emit configured();
    }
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::setDefaultColor()
{
    Options::instance().setDefaultColor();

    M_color_list_box->clear();
    createColorItems();

    emit configured();
}

/*-------------------------------------------------------------------*/
/*!

*/
void
ConfigDialog::cancelColor()
{
    for ( int i = 0; i < M_color_list_box->count(); ++i )
    {
        ColorItem * color_item = dynamic_cast< ColorItem * >( M_color_list_box->item( i ) );
        if ( ! color_item )
        {
            continue;
        }

        color_item->revert();
    }

    emit configured();
}
