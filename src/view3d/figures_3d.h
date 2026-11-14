// -*-c++-*-

/*!
  \file figures_3d.h
  \brief the players of the 3D view as human figures. Header File.
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

#ifndef RCSSMONITOR_FIGURES_3D_H
#define RCSSMONITOR_FIGURES_3D_H

#include "gltf_model.h"
#include "humanoid.h"
#include "scene_content.h"

#include <QMatrix4x4>
#include <QString>
#include <QStringList>

#include <vector>

/*!
  \class Figures3D
  \brief the players as human figures (docs/PLAN_3D-VIEW.md §5.8): the
  bundled model, the Mannequin of Quaternius's Universal Animation Library
  (CC0, resources/models), and the poses of the figures that SceneBuilder
  put in a SceneContent. No OpenGL: Scene3D draws them.

  A pose is decided from the log every frame, from the frame and the
  fraction to the next one only, so that a frame looks the same however it
  is reached (paused, stepped back, jumped to). The motion of a clip goes
  through the humanoid layer (HumanoidRig): read as a HumanoidPose and put
  on the model, so that motions and models of other skeletons can come
  later (the figure packs, §5.9).
*/
class Figures3D {
public:

    //! how far apart the players' idle clips are (s), so that they do not move together
    static constexpr double IDLE_OFFSET = 0.53;

    /*!
      \brief read the bundled model (once)
      \return false if it cannot be read; error() tells why
    */
    bool load();

    bool ready() const { return M_ready; }
    const QString & error() const { return M_error; }

    //! the materials of the model in each figure's jersey color
    static QStringList jerseyMaterials();

    const GltfModel & model() const { return M_model; }
    const HumanoidRig & rig() const { return M_rig; }

    //! where a figure is in the world: standing on the ground at the player, facing its body direction, as high as figure.height
    QMatrix4x4 modelMatrix( const FigureInstance & figure ) const;

    /*!
      \brief the transforms of the model's nodes for a figure
      \param figure the figure
      \param time the game's time (s), with the fraction to the next frame
      \param local by node index
    */
    void localPose( const FigureInstance & figure,
                    const double time,
                    std::vector< NodeTransform > & local ) const;

    /*!
      \brief the skin matrices of the figures of a content, into
      content.figure_joints (in the order of content.figures)
      \param content the figures (SceneBuilder::addShow())
      \param time the game's time (s), with the fraction to the next frame
    */
    void pose( SceneContent & content,
               const double time ) const;

private:
    bool M_loaded = false; //!< load() was tried
    bool M_ready = false;
    QString M_error;
    GltfModel M_model;
    HumanoidRig M_rig;
    int M_idle = -1; //!< the clip of standing
};

#endif
