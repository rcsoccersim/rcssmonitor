// -*-c++-*-

/*!
  \file field_view_3d.h
  \brief the 3D view: a QOpenGLWidget. Header File.
*/

/*
 *Copyright:

 Copyright (C) The RoboCup Soccer Server Maintenance Group.
 Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
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

#ifndef RCSSMONITOR_FIELD_VIEW_3D_H
#define RCSSMONITOR_FIELD_VIEW_3D_H

#include "scene_3d.h"
#include "scene_builder.h"

#include <QElapsedTimer>
#include <QOpenGLWidget>
#include <QString>
#include <QSurfaceFormat>

#include <memory>

class DispHolder;
class FieldPainter;
class LogPlayer;

namespace rcss {
namespace rcg {
struct DispInfoT;
}
}

/*!
  \class FieldView3D
  \brief the 3D view (docs/PLAN_3D-VIEW.md §5.1): the widget, which draws
  the scene of Scene3D and handles the input. It shares the frames
  (DispHolder) and the options with the 2D view. Between two frames, it
  draws the positions on the way (docs/PLAN_3D-VIEW.md §5.2), and while they
  move, it draws again as soon as a frame is shown.
*/
class FieldView3D
    : public QOpenGLWidget {

    Q_OBJECT

public:

    explicit
    FieldView3D( DispHolder & disp_holder,
                 const LogPlayer & log_player,
                 QWidget * parent = nullptr );
    ~FieldView3D() override;

    //! OpenGL 3.3 core with 4x multisampling
    static QSurfaceFormat surfaceFormat();

    /*!
      \brief check if the 3D view can be shown here.
      \return an empty string if it can, or why it cannot
    */
    static QString unavailableReason();

protected:

    void initializeGL() override;
    void paintGL() override;

private:

    DispHolder & M_disp_holder;
    const LogPlayer & M_log_player;
    std::unique_ptr< FieldPainter > M_field_painter;
    Scene3D M_scene;
    SceneContent M_content; //!< reused in every frame
    bool M_ready;
    QString M_ground_key; //!< what the ground was drawn with

    const rcss::rcg::DispInfoT * M_newest; //!< the newest frame when it was last drawn
    QElapsedTimer M_newest_clock; //!< since the newest frame came
    bool M_moving; //!< to be drawn again in the next frame

private slots:
    void drawNext();
};

#endif
