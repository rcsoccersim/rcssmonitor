// -*-c++-*-

/*!
  \file config_dialog.h
  \brief Config Dialog class Header File.
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

#ifndef RCSSMONITOR_CONFIG_DIALOG_H
#define RCSSMONITOR_CONFIG_DIALOG_H

#include <QDialog>
#include <QListWidgetItem>
#include <QPushButton>

#include <boost/function.hpp>

#include <vector>

class QPushButton;
class QCheckBox;
class QCloseEvent;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QRadioButton;
class QSlider;
class QSpinBox;
class QTabWidget;

class ConfigDialog;
class DispHolder;


/*

 */
class ColorItem
    : public QListWidgetItem {
public:
    typedef boost::function< void ( const QColor & ) > Func;

private:
    QColor M_new_color;
    QColor M_old_color;
    Func M_func;

public:
    ColorItem( const QString & name,
               const QColor & old_color,
               Func func,
               QListWidget * parent = 0 );

    const QColor & newColor() const { return M_new_color; }

    bool setColor( const QColor & color );
    void updateColor();
    bool revert();
};


/*

 */
class FontButton
    : public QPushButton {

    Q_OBJECT

public:

    typedef boost::function< void ( const QFont & ) > Func;

private:
    QString M_name;
    QFont M_new_font;
    QFont M_old_font;
    Func M_func;

public:

    FontButton( const QString & name,
                const QFont & old_font,
                Func func,
                ConfigDialog * parent );

    void setNewFont( const QFont & font );

    void revert();
    void updateText();

private slots:

    void fontDialog();

signals:

    void fontChanged();
};


/*

 */
class ConfigDialog
    : public QDialog {

    Q_OBJECT

private:

    const DispHolder & M_disp_holder;

    QTabWidget * M_tab_widget;

    // zoom control
    QSlider * M_scale_slider;
    QLineEdit * M_scale_text;

    // object size
    QLineEdit * M_ball_size_text;
    QLineEdit * M_player_size_text;

    // canvas size control
    QLineEdit * M_canvas_width_text;
    QLineEdit * M_canvas_height_text;

    // misc options
    QCheckBox * M_anti_aliasing_cb;

    // show/hide control
    QCheckBox * M_show_score_board_cb;
    QCheckBox * M_show_keepaway_area_cb;
    QCheckBox * M_show_team_graphic_cb;
    QCheckBox * M_show_flag_cb;

    QCheckBox * M_show_ball_cb;
    QCheckBox * M_show_player_cb;

    QCheckBox * M_player_number_cb;
    QCheckBox * M_player_type_cb;
    QCheckBox * M_stamina_cb;
    QCheckBox * M_stamina_capacity_cb;
    QCheckBox * M_view_area_cb;
    QCheckBox * M_catch_area_cb;
    QCheckBox * M_tackle_area_cb;
    QCheckBox * M_kick_accel_area_cb;
    QCheckBox * M_pointto_cb;
    QCheckBox * M_card_cb;

    QCheckBox * M_show_offside_line_cb;
    QCheckBox * M_show_draw_info_cb;

    QSlider * M_grid_step_slider;
    QLineEdit * M_grid_step_text;
    QCheckBox * M_show_grid_coord_cb;

    // focus control
    QRadioButton * M_focus_ball_rb;
    QRadioButton * M_focus_player_rb;
    QRadioButton * M_focus_fix_rb;

    // player selection
    QRadioButton * M_select_all_rb;
    QRadioButton * M_select_left_rb;
    QRadioButton * M_select_right_rb;
    QRadioButton * M_select_fix_rb;
    QRadioButton * M_unselect_rb;

    QComboBox * M_player_choice;

//     // trace control
//     QLineEdit * M_ball_trace_start;
//     QLineEdit * M_ball_trace_end;
//     QLineEdit * M_player_trace_start;
//     QLineEdit * M_player_trace_end;

    // inertia movement control
    QSpinBox * M_ball_vel_cycle;

    // color list
    QListWidget * M_color_list_box;

public:

    ConfigDialog( QWidget * parent,
                  const DispHolder & disp_holder );
    ~ConfigDialog();
private:

    void createWidgets();

    QWidget * createZoomControls();
    QWidget * createObjectSizeControls();
    QWidget * createCanvasSizeControls();
    QWidget * createPlayerInfoControls();
    QWidget * createBallInfoControls();
    QWidget * createShowControls();
    QWidget * createFieldStyleControls();
    QWidget * createMiscControls();
    QWidget * createGridStepControls();
    QWidget * createFocusControls();
    QWidget * createPlayerSelectionControls();
    QLayout * createColorList();
    void createColorItems();
    QLayout * createFontButtons();

protected:

    void showEvent( QShowEvent * event );


private slots:

    // just update this dialog contents
    // no emit
    void updateAll();

    //-----------------------

    // connect to itself
    // emit configured()
    void slideFieldScale( int value );
    void editFieldScale( const QString & );

    void clickShowPlayerNumber( bool checked );
    void clickShowPlayerType( bool checked );
    void clickShowStamina( bool checked );
    void clickShowStaminaCapacity( bool checked );
    void clickShowViewArea( bool checked );
    void clickShowCatchArea( bool checked );
    void clickShowTackleArea( bool checked );
    void clickShowKickAccelArea( bool checked );
    void clickShowPointto( bool checked );
    void clickShowCard( bool checked );

    void clickShowScoreBoard( bool checked );
    void clickShowKeepawayArea( bool checked );
    void clickShowTeamGraphic( bool checked );
    void clickShowDrawInfo( bool checked );

    void clickShowBall( bool checked );
    void clickShowPlayer( bool checked );
    void clickShowFlag( bool checked );
    void clickShowOffsideLine( bool checked );

    void clickShowGridCoord( bool checked );
    void slideGridStep( int value );
    void editGridStep( const QString & text );

    void clickAntiAliasing( bool checked );

    void clickFocusBall();
    void clickFocusPlayer();

    void clickSelectAutoAll();
    void clickSelectAutoLeft();
    void clickSelectAutoRight();
    void clickSelectFix();

    void choicePlayer( int number );

//     void editBallTraceStart( const QString & text );
//     void editBallTraceEnd( const QString & text );
//     void clickBallTraceAll();

//     void editPlayerTraceStart( const QString & text );
//     void editPlayerTraceEnd( const QString & text );
//     void clickPlayerTraceAll();

//     void clickLinePointButton();

    void changeBallVelCycle( int value );

    void selectColorEntry( QListWidgetItem * item );
    void setDefaultColor();
    void cancelColor();

public slots:

    // just update this dialog contents
    // no emit
    void updateFieldScale();

    // connect to other objects
    // emit configured()
    void zoomIn();
    void zoomOut();
    void fitToScreen();

    void applyCanvasSize();

    void toggleShowScoreBoard();
    void toggleShowKeepawayArea();
    void toggleShowTeamGraphic();
    void toggleShowFlag();

    void toggleShowPlayer();
    void toggleShowBall();

    void toggleShowPlayerNumber();
    void toggleShowPlayerType();
    void toggleShowStamina();
    void toggleShowStaminaCapacity();
    void toggleShowViewArea();
    void toggleShowCatchArea();
    void toggleShowTackleArea();
    void toggleShowKickAccelArea();
    void toggleShowPointto();
    void toggleShowCard();

    void toggleShowOffsideLine();
    void toggleShowDrawInfo();

    void editBallSize( const QString & text );
    void editPlayerSize( const QString & text );

    void toggleFocusBall();
    void toggleFocusPlayer();
    void setFocusFix();
    void setFocusPoint( const QPoint & point );

    void toggleSelectAutoAll();
    void toggleSelectAutoLeft();
    void toggleSelectAutoRight();
    void toggleSelectFix();
    void setUnselect();

    void selectPlayerWithKey();
    void selectPlayer( int number );

signals:

    void configured();
    void canvasResized( const QSize & size );
};

#endif
