// -*-c++-*-

/*!
  \file fbx_model.h
  \brief an FBX file read into a GltfModel, with ufbx. Header File.
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

#ifndef RCSSMONITOR_FBX_MODEL_H
#define RCSSMONITOR_FBX_MODEL_H

#include <QByteArray>
#include <QString>

class GltfModel;

/*!
  \brief FBX files (docs/plan/2026-11-11_3d-view.md §5.9, D12), read with ufbx
  (third_party/ufbx) into the same plain data as a glTF file (GltfModel),
  so that the rest (the humanoid layer, the figures, the drawing) takes
  them alike: a model downloaded from Mixamo goes in as it is.

  The scene is converted to glTF's space: y up, a person facing +z, in
  meters (FBX files are often in centimeters, z up, or turned by their
  root). The nodes keep their names (Mixamo's "mixamorig:Hips"); their
  rest is the bind pose of the skin where it has one. Each mesh of a node
  is read, skinned by its skin (its clusters are the joints) or fixed to
  its node, in triangles, by its materials: the base color, or the diffuse
  color, and its texture (embedded in the file, or a file beside it). A
  texture with transparent pixels is a mask; the takes are the clips,
  baked at 30 keys a second.
*/
namespace FbxModel {

//! whether a file is FBX: by its header (binary or ASCII), or by the extension of its name
bool isFbx( const QByteArray & data,
            const QString & path );

/*!
  \brief read an FBX file into a model (cleared first): its nodes, joints,
  meshes, materials and textures, and clips. GltfModel::load() calls it
  and does the rest (the rest pose of the mesh).
  \param data the file
  \param path where it is, to find the textures beside it (empty: none)
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
