// -*-c++-*-

/*!
  \file figures_3d.cpp
  \brief the players of the 3D view as human figures. Source File.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "figures_3d.h"

#include <QFile>

#include <algorithm>

namespace {

//! the bundled model in the resources (resources/models/view3d_models.qrc)
const char * MODEL_PATH = ":/models/Mannequin.glb";
//! its clip of standing
const char * IDLE_CLIP = "Idle_Loop";

}

/*-------------------------------------------------------------------*/
bool
Figures3D::load()
{
    if ( M_loaded ) return M_ready;
    M_loaded = true;

    // the resources of a static library must be registered before use
    Q_INIT_RESOURCE( view3d_models );

    QFile file( QString::fromLatin1( MODEL_PATH ) );
    if ( ! file.open( QIODevice::ReadOnly ) )
    {
        M_error = QStringLiteral( "cannot open " ) + file.fileName();
        return false;
    }
    QString error;
    if ( ! M_model.load( file.readAll(), &error ) )
    {
        M_error = file.fileName() + QStringLiteral( ": " ) + error;
        return false;
    }
    if ( ! M_rig.build( M_model, quaterniusBoneNames(), &error ) )
    {
        M_error = file.fileName() + QStringLiteral( ": " ) + error;
        return false;
    }
    M_idle = M_model.clipIndex( QString::fromLatin1( IDLE_CLIP ) );
    if ( M_idle < 0 )
    {
        M_error = file.fileName() + QStringLiteral( ": no clip " ) + IDLE_CLIP;
        return false;
    }
    M_ready = true;
    return true;
}

/*-------------------------------------------------------------------*/
QStringList
Figures3D::jerseyMaterials()
{
    // the Mannequin's body; its joints (M_Joints) keep their own color
    return QStringList( { QStringLiteral( "M_Main" ) } );
}

/*-------------------------------------------------------------------*/
QMatrix4x4
Figures3D::modelMatrix( const FigureInstance & figure ) const
{
    // A glTF person faces +z. The field's angle d (from +x toward +y, which
    // is the world's +z) is +x turned about y by -d, and +z turned by 90 - d.
    QMatrix4x4 m;
    m.translate( figure.x, 0.0f, figure.y );
    m.rotate( 90.0f - figure.body, 0.0f, 1.0f, 0.0f );
    m.scale( figure.height / std::max( 0.01f, M_model.height() ) );
    m.translate( 0.0f, -M_model.min_y, 0.0f ); // the feet on the ground
    return m;
}

/*-------------------------------------------------------------------*/
void
Figures3D::localPose( const FigureInstance & figure,
                      const double time,
                      std::vector< NodeTransform > & local ) const
{
    local = M_model.rest;
    if ( ! M_ready ) return;

    // standing (7-1): the idle clip at the game's time, each player at
    // its own point of it
    std::vector< NodeTransform > motion = M_model.rest;
    M_model.sample( M_model.clips[M_idle],
                    static_cast< float >( time + figure.player * IDLE_OFFSET ),
                    motion );

    // through the humanoid layer: the motion's skeleton is the model's, so
    // the bones without a role (the fingers) take the motion's too
    const HumanoidPose pose = M_rig.poseOf( motion );
    local = motion;
    M_rig.apply( pose, local );
}

/*-------------------------------------------------------------------*/
void
Figures3D::pose( SceneContent & content,
                 const double time ) const
{
    content.figure_joints.clear();
    if ( ! M_ready ) return;

    const std::size_t floats = static_cast< std::size_t >( M_model.jointCount() ) * 16;
    content.figure_joints.resize( content.figures.size() * floats );
    std::vector< NodeTransform > local;
    for ( std::size_t k = 0; k < content.figures.size(); ++k )
    {
        const FigureInstance & figure = content.figures[k];
        localPose( figure, time, local );
        M_model.skin( local, modelMatrix( figure ), &content.figure_joints[k * floats] );
    }
}
