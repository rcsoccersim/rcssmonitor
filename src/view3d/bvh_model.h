// -*-c++-*-

/*!
  \file bvh_model.h
  \brief a BVH file (motion capture) read into a GltfModel. Header File.
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

#ifndef RCSSMONITOR_BVH_MODEL_H
#define RCSSMONITOR_BVH_MODEL_H

#include <QByteArray>
#include <QString>

class GltfModel;

/*!
  \brief BVH files (Biovision Hierarchy, of motion capture: docs/PLAN_3D-VIEW.md
  §5.9, D12), read into the same plain data as a glTF file (GltfModel):
  a skeleton without a mesh, and its motion as one clip.

  The hierarchy gives the nodes (each ROOT and JOINT, and each End Site
  as "<its joint>_End") and their rest: moved by their OFFSET, not turned.
  The motion gives a key a frame: the rotation channels turn a node in
  the order written (Zrotation Xrotation Yrotation: R = Rz Rx Ry, in
  degrees), and the position channels place it in its parent (in place of
  its OFFSET), usually the root's only. The values are kept in the file's
  unit, and its axes as they are: BVH sets neither, and a figure takes
  lengths relative to the hips' height, and turns a skeleton upright and
  to face +z by itself (Figures3D).
*/
namespace BvhModel {

//! whether a file is BVH: by its header (HIERARCHY), or by the extension of its name
bool isBvh( const QByteArray & data,
            const QString & path );

/*!
  \brief read a BVH file into a model (cleared first): its nodes and their
  rest, and its motion as one clip named after the file (empty path:
  "motion"), from 0 s, a key a frame. A file cut short in its motion keeps
  the frames it has whole.
  \param data the file
  \param path where it is, for the clip's name (empty: none)
  \param model the model
  \param error what went wrong, if not null
  \return false if it cannot be read
*/
bool read( const QByteArray & data,
           const QString & path,
           GltfModel & model,
           QString * error );

}

#endif
