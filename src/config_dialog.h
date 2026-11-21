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

#include <functional>
#include <utility>
#include <vector>

class QPushButton;
class QAction;
class QButtonGroup;
class QCheckBox;
class QCloseEvent;
class QComboBox;
class QDoubleSpinBox;
class QGridLayout;
class QLabel;
class QLineEdit;
class QListWidget;
class QToolButton;
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
    typedef std::function< void ( const QColor & ) > Setter;

private:
    QColor M_new_color;
    QColor M_old_color;
    Setter M_setter;

public:
    ColorItem( const QString & name,
               const QColor & old_color,
               Setter setter,
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

    typedef std::function< void ( const QFont & ) > Setter;

private:
    QString M_name;
    QFont M_new_font;
    QFont M_old_font;
    Setter M_setter;

public:

    FontButton( const QString & name,
                const QFont & old_font,
                Setter setter,
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
    QLineEdit * M_focus_point_size_text;

    // canvas size control
    QLineEdit * M_canvas_width_text;
    QLineEdit * M_canvas_height_text;

    QDoubleSpinBox * M_team_graphic_scale;

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
    QCheckBox * M_focus_point_cb;
    QCheckBox * M_all_focus_points_cb;
    QCheckBox * M_catch_area_cb;
    QCheckBox * M_tackle_area_cb;
    QCheckBox * M_kick_accel_area_cb;
    QCheckBox * M_pointto_cb;
    QCheckBox * M_card_cb;
    QCheckBox * M_illegal_defense_cb;

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

    // the 3D tab (only in a build with the 3D view; null otherwise)
    QDoubleSpinBox * M_closeup_distance_3d = nullptr;
    QDoubleSpinBox * M_closeup_elevation_3d = nullptr;
    QButtonGroup * M_compass_3d = nullptr;
    QButtonGroup * M_minimap_3d = nullptr;
    QComboBox * M_player_model_3d = nullptr; //!< the cylinders or a figure pack (fillPacks())
    QToolButton * M_figure_folder_3d = nullptr; //!< the folders of the figure packs
    QAction * M_no_figure_folder_3d = nullptr;
    QMenu * M_trash_figure_pack_3d = nullptr; //!< the user's figure packs, to move to the trash (fillPacks())
    QLabel * M_figure_credits_3d = nullptr; //!< the pack's credits, or why it cannot be drawn
    QButtonGroup * M_figure_bodies_3d = nullptr;
    QLabel * M_figure_bodies_3d_label = nullptr;
    QDoubleSpinBox * M_figure_scale_3d = nullptr;
    QLabel * M_figure_scale_3d_label = nullptr;
    QButtonGroup * M_shadows_3d = nullptr;
    QCheckBox * M_boards_3d_cb = nullptr;
    QCheckBox * M_banners_3d_cb = nullptr;
    QButtonGroup * M_grass_3d = nullptr;
    QCheckBox * M_stands_3d_cb = nullptr;
    QCheckBox * M_ball_pattern_3d_cb = nullptr;
    QCheckBox * M_fps_3d_cb = nullptr;
    QCheckBox * M_antialias_3d_cb = nullptr;
    QCheckBox * M_ball_flight_3d_cb = nullptr;
    QCheckBox * M_ball_ring_3d_cb = nullptr;
    QCheckBox * M_ball_trail_3d_cb = nullptr;
    QDoubleSpinBox * M_ball_size_3d = nullptr;
    QDoubleSpinBox * M_ball_ring_radius_3d = nullptr;
    QLabel * M_ball_ring_radius_3d_label = nullptr;
    QDoubleSpinBox * M_ball_trail_cycles_3d = nullptr;
    QLabel * M_ball_trail_cycles_3d_label = nullptr;
    QCheckBox * M_sky_3d_cb = nullptr;

    // the delayed live view (the Show tab, §5.10)
    QCheckBox * M_live_delay_cb = nullptr;
    QDoubleSpinBox * M_live_delay_seconds = nullptr;

    // the Player Camera tab (only in a build with the 3D view; null otherwise)
    QButtonGroup * M_player_camera_position = nullptr;
    QButtonGroup * M_player_camera_direction = nullptr;
    QButtonGroup * M_player_camera_motion = nullptr;
    QButtonGroup * M_player_camera_fov = nullptr;
    QButtonGroup * M_player_camera_sight = nullptr;
    QDoubleSpinBox * M_player_camera_time_constant = nullptr;
    QDoubleSpinBox * M_player_camera_max_turn = nullptr;
    QDoubleSpinBox * M_player_camera_fixed_fov = nullptr;
    QLabel * M_player_camera_time_constant_label = nullptr;
    QLabel * M_player_camera_max_turn_label = nullptr;
    QLabel * M_player_camera_fixed_fov_label = nullptr;

public:

    ConfigDialog( QWidget * parent,
                  const DispHolder & disp_holder );
    ~ConfigDialog();

    //! the fields of the delayed live view from the options (turned on or off in the tool bar)
    void updateLiveDelay();
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
    QWidget * createLiveControls();
    QLayout * createColorList();
    void createColorItems();
    QLayout * createFontButtons();
    QWidget * create3DTab();
    QWidget * createPlayersBallTab();
    QWidget * createCameraTab();

    QButtonGroup * addChoices( QGridLayout * layout,
                               const QString & label,
                               const QString & tooltip,
                               const std::vector< std::pair< QString, QString > > & choices );
    QDoubleSpinBox * addNumber( QGridLayout * layout,
                                int row,
                                int column,
                                const QString & label,
                                const QString & tooltip,
                                double min,
                                double max,
                                double step,
                                int decimals,
                                const QString & unit,
                                QLabel ** label_widget = nullptr );
    void changed3D();
    //! the cylinders and the figure packs found, the one of the options chosen, and its credits
    void fillPacks();

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

    void slotTeamGraphicScaleChanged( double value );

    void clickShowPlayerNumber( bool checked );
    void clickShowPlayerType( bool checked );
    void clickShowStamina( bool checked );
    void clickShowStaminaCapacity( bool checked );
    void clickShowViewArea( bool checked );
    void clickShowFocusPoint( bool checked );
    void clickShowAllFocusPoints( bool checked );
    void clickShowCatchArea( bool checked );
    void clickShowTackleArea( bool checked );
    void clickShowKickAccelArea( bool checked );
    void clickShowPointto( bool checked );
    void clickShowCard( bool checked );
    void clickShowIllegalDefense( bool checked );

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
    // the 3D tab and the Player Camera tab from the options
    void update3D();
    // a folder of figure packs of the user's choice (empty: none); the packs looked for again
    // emit configured()
    void setFigureFolder( const QString & folder );
    // a figure pack of the user's (by its folder) moved to the trash, if the user says so; not
    // drawn any more (the cylinders if it was chosen), the packs looked for again
    // emit configured()
    void trashFigurePack( const QString & dir );

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
    void toggleShowFocusPoint();
    void toggleShowCatchArea();
    void toggleShowTackleArea();
    void toggleShowKickAccelArea();
    void toggleShowPointto();
    void toggleShowCard();
    void toggleShowIllegalDefense();

    void toggleShowOffsideLine();
    void toggleShowDrawInfo();

    void editBallSize( const QString & text );
    void editPlayerSize( const QString & text );
    void editFocusPointSize( const QString & text );

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
    //! the options of the delayed live view changed (§5.10)
    void liveDelayChanged();
};

#endif
