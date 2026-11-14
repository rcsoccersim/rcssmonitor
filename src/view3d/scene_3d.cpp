// -*-c++-*-

/*!
  \file scene_3d.cpp
  \brief the scene of the 3D view, drawn with OpenGL 3.3 core. Source File.
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

#include "scene_3d.h"

#include "ground_texture.h"
#include "meshes_3d.h"
#include "stadium_3d.h"
#include "gl/frame.h"
#include "gl/render_state.h"

#include <QVector3D>

#include <algorithm>

namespace {

// the dashes and the gaps of a dotted line (m), and the size of a dot (pixels), as rcssmonitor-web
constexpr float DASH = 0.3f;
constexpr float POINT_SIZE = 4.0f;

// the color of the background: the sky, until it is drawn (Phase 8)
const QColor SKY = QColor::fromRgbF( 0.55, 0.72, 0.88 );

// toward the sun: from the far left, 44 degrees up (rcssmonitor-web, §4.3)
const QVector3D SUN = QVector3D( -60.0f, 65.0f, -30.0f ).normalized();

}

/*-------------------------------------------------------------------*/
Scene3D::Scene3D()
    : M_ground_instance( 1, Instance3D{ QMatrix4x4(), QVector4D( 1.0f, 1.0f, 1.0f, 1.0f ) } )
{

}

/*-------------------------------------------------------------------*/
Scene3D::~Scene3D() = default;

/*-------------------------------------------------------------------*/
bool
Scene3D::initialize()
{
    if ( M_initialized )
    {
        return true;
    }

    // find the OpenGL 3.3 functions in the current context
    if ( ! initializeOpenGLFunctions() )
    {
        return false;
    }

    // compile the shaders
    if ( ! M_ground_program.load( "ground" )
         || ! M_lit_program.load( "lit" )
         || ! M_flat_program.load( "flat" )
         || ! M_billboard_program.load( "billboard" )
         || ! M_colored_program.load( "colored" )
         || ! M_panel_program.load( "panel" )
         || ! M_skinned_program.load( "skinned", "lit" ) )
    {
        M_ground_program.destroy();
        M_lit_program.destroy();
        M_flat_program.destroy();
        M_billboard_program.destroy();
        M_colored_program.destroy();
        M_panel_program.destroy();
        M_skinned_program.destroy();
        return false;
    }

    // send the shapes to the GPU, once. The numbers are how finely the
    // round ones are divided into triangles.
    M_ground.create( *this, Meshes3D::ground( float( GroundTexture::length() ), float( GroundTexture::width() ) ) );
    M_cylinder.create( *this, Meshes3D::cylinder( 20 ) );
    M_sphere.create( *this, Meshes3D::sphere( 24, 16 ) );
    M_cone.create( *this, Meshes3D::cone( 12 ) );
    M_disc.create( *this, Meshes3D::ring( 0.0f, 1.0f, 32 ) );
    M_ring.create( *this, Meshes3D::ring( 0.92f, 1.08f, 40 ) );
    M_decay_ring.create( *this, Meshes3D::ring( 1.1f, 1.22f, 40 ) );
    M_quad.create( *this, Meshes3D::quad() );
    M_panel.create( *this, Meshes3D::quad() );
    M_billboards.create( *this );
    for ( VertexBatch * b : { &M_sector_batch, &M_line_batch, &M_dotted_batch, &M_point_batch } )
    {
        b->create( *this );
    }
    M_initialized = true;
    return true;
}

/*-------------------------------------------------------------------*/
void
Scene3D::release()
{
    if ( ! M_initialized )
    {
        return;
    }
    for ( InstancedMesh * m : { &M_ground, &M_cylinder, &M_sphere, &M_cone, &M_disc, &M_ring, &M_decay_ring, &M_quad, &M_panel } )
    {
        m->destroy( *this );
    }
    M_billboards.destroy( *this );
    M_figure.destroy( *this );
    for ( VertexBatch * b : { &M_sector_batch, &M_line_batch, &M_dotted_batch, &M_point_batch } )
    {
        b->destroy( *this );
    }
    M_ground_texture.destroy( *this );
    M_label_texture.destroy( *this );
    for ( int end = 0; end < 2; ++end )
    {
        M_board_textures[end].destroy( *this );
        M_banner_textures[end].destroy( *this );
    }
    M_ground_program.destroy();
    M_lit_program.destroy();
    M_flat_program.destroy();
    M_billboard_program.destroy();
    M_colored_program.destroy();
    M_panel_program.destroy();
    M_skinned_program.destroy();
    M_initialized = false;
}

/*-------------------------------------------------------------------*/
void
Scene3D::setGround( const QImage & image )
{
    M_ground_texture.upload( *this, image );
}

/*-------------------------------------------------------------------*/
void
Scene3D::setLabels( const QImage & image )
{
    M_label_texture.upload( *this, image );
}

/*-------------------------------------------------------------------*/
void
Scene3D::setStadium( const int end,
                     const QImage & board,
                     const QImage & banner )
{
    M_board_textures[end].upload( *this, board, true ); // repeated along the boards
    M_banner_textures[end].upload( *this, banner );
}

/*-------------------------------------------------------------------*/
void
Scene3D::setFigureModel( const GltfModel & model,
                         const QStringList & jersey )
{
    if ( ! M_initialized )
    {
        return;
    }
    M_figure.destroy( *this );
    M_figure.create( *this, model, jersey );
}

/*-------------------------------------------------------------------*/
void
Scene3D::render( GLuint fbo,
                 int width,
                 int height,
                 const CameraPose & camera,
                 const SceneContent & content )
{
    if ( ! M_initialized )
    {
        return;
    }

    // the camera: one matrix that takes a point of the world to the screen
    const QMatrix4x4 view_proj = camera.projection( float( width ) / std::max( 1, height ) ) * camera.view();

    // start the frame: into the framebuffer, cleared to the sky
    GLFrame::begin( *this, fbo, width, height, SKY );

    //
    // pass 1: the ground, with the image of the 2D view on texture unit 0
    //
    RenderState::opaque().apply( *this );
    M_ground_program.bind();
    M_ground_program.set( "uViewProj", view_proj );
    M_ground_program.set( "uTexture", 0 ); // the texture unit
    M_ground_texture.bind( *this, 0 );
    M_ground.draw( *this, M_ground_instance );

    //
    // pass 2: the solids, lit by the sun (the same render state)
    //
    M_lit_program.bind();
    M_lit_program.set( "uViewProj", view_proj );
    M_lit_program.set( "uSun", SUN );
    M_cylinder.draw( *this, content.cylinders );
    M_sphere.draw( *this, content.spheres );
    M_cone.draw( *this, content.cones );

    //
    // pass 2a: the human figures, lit as the solids, their skeletons posed
    // by the skin matrices (Figures3D), each in its jersey
    //
    if ( ! content.figures.empty() )
    {
        M_figure_colors.clear();
        for ( const FigureInstance & f : content.figures )
        {
            M_figure_colors.push_back( f.color );
        }
        M_skinned_program.bind();
        M_skinned_program.set( "uViewProj", view_proj );
        M_skinned_program.set( "uSun", SUN );
        M_figure.draw( *this, M_skinned_program, M_figure_colors, content.figure_joints );
    }

    //
    // pass 2b: the boards and the banners, solid and unlit, both faces (the
    // shader paints the back plain), with each end's images on texture unit 0
    //
    RenderState::opaqueTwoSided().apply( *this );
    M_panel_program.bind();
    M_panel_program.set( "uViewProj", view_proj );
    M_panel_program.set( "uTexture", 0 );
    for ( int end = 0; end < 2; ++end )
    {
        if ( ! content.boards[end].empty() )
        {
            M_panel_program.set( "uTile", Stadium3D::BOARD_TILE );
            M_board_textures[end].bind( *this, 0 );
            M_panel.draw( *this, content.boards[end] );
        }
        if ( ! content.banners[end].empty() )
        {
            M_panel_program.set( "uTile", Stadium3D::BANNER_WIDTH );
            M_banner_textures[end].bind( *this, 0 );
            M_panel.draw( *this, content.banners[end] );
        }
    }

    //
    // pass 3: the translucent ones, after the solids, which hide them where
    // nearer. They do not write the depth, so the order among them decides
    // which is over which: the layers on the ground from low to high (the
    // circles, the rings, the view areas, the lines and dots), then the nets.
    //
    RenderState::translucent().apply( *this );
    M_flat_program.bind();
    M_flat_program.set( "uViewProj", view_proj );
    M_disc.draw( *this, content.discs );
    M_ring.draw( *this, content.rings );
    M_decay_ring.draw( *this, content.decay_rings );

    M_colored_program.bind();
    M_colored_program.set( "uViewProj", view_proj );
    M_colored_program.set( "uPointSize", POINT_SIZE );
    M_colored_program.set( "uDash", 0.0f );
    M_sector_batch.draw( *this, content.sectors, VertexBatch::Primitive::Triangles );
    M_line_batch.draw( *this, content.lines, VertexBatch::Primitive::Lines );
    M_point_batch.draw( *this, content.points, VertexBatch::Primitive::Points );
    M_colored_program.set( "uDash", DASH );
    M_dotted_batch.draw( *this, content.dotted_lines, VertexBatch::Primitive::Lines );

    M_flat_program.bind();
    M_quad.draw( *this, content.quads );

    //
    // pass 4: the labels, over everything (as the 2D view writes the
    // numbers over the players), facing the camera, from far to near
    //
    if ( ! content.labels.empty() )
    {
        const QMatrix4x4 view = camera.view();
        RenderState::overlay().apply( *this );
        M_billboard_program.bind();
        M_billboard_program.set( "uViewProj", view_proj );
        // the rows of the view matrix: the camera's right and up in the world
        M_billboard_program.set( "uRight", QVector3D( view( 0, 0 ), view( 0, 1 ), view( 0, 2 ) ) );
        M_billboard_program.set( "uUp", QVector3D( view( 1, 0 ), view( 1, 1 ), view( 1, 2 ) ) );
        M_billboard_program.set( "uTexture", 0 );
        M_label_texture.bind( *this, 0 );
        M_billboards.draw( *this, content.labels );
    }

    // end the frame: OpenGL as QPainter expects it, for the HUD over the scene
    GLFrame::end( *this );
}
