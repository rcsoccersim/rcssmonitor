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

#include "field_painter.h"
#include "ground_texture.h"
#include "meshes_3d.h"
#include "grass_3d.h"
#include "scenery_3d.h"
#include "stadium_3d.h"
#include "sun_3d.h"
#include "gl/frame.h"
#include "gl/render_state.h"

#include <QVector3D>

#include <algorithm>

namespace {

// the dashes and the gaps of a dotted line (m), and the size of a dot (pixels), as rcssmonitor-web
constexpr float DASH = 0.3f;
constexpr float POINT_SIZE = 4.0f;

QVector3D
rgb( const QColor & c )
{
    return QVector3D( float( c.redF() ), float( c.greenF() ), float( c.blueF() ) );
}

// the texture unit of the shadow map (unit 0 has the ground's image or the
// figures' skin matrices), and of the realistic grass's noise
constexpr int SHADOW_UNIT = 1;
constexpr int NOISE_UNIT = 2;

QMatrix4x4
shadow_plane_matrix()
{
    QMatrix4x4 m;
    m.translate( 0.0f, Sun3D::SHADOW_PLANE_Y, 0.0f );
    return m;
}

}

/*-------------------------------------------------------------------*/
Scene3D::Scene3D()
    : M_ground_instance( 1, Instance3D{ QMatrix4x4(), QVector4D( 1.0f, 1.0f, 1.0f, 1.0f ) } ),
      M_shadow_instance( 1, Instance3D{ shadow_plane_matrix(), QVector4D( 0.0f, 0.0f, 0.0f, 1.0f ) } )
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
         || ! M_skinned_program.load( "skinned" )
         || ! M_depth_program.load( "depth" )
         || ! M_skinned_depth_program.load( "skinned", "depth" )
         || ! M_shadow_program.load( "shadow" )
         || ! M_blob_program.load( "blob" )
         || ! M_sky_program.load( "sky" )
         || ! M_outside_program.load( "outside" ) )
    {
        M_ground_program.destroy();
        M_lit_program.destroy();
        M_flat_program.destroy();
        M_billboard_program.destroy();
        M_colored_program.destroy();
        M_panel_program.destroy();
        M_skinned_program.destroy();
        M_depth_program.destroy();
        M_skinned_depth_program.destroy();
        M_shadow_program.destroy();
        M_blob_program.destroy();
        M_sky_program.destroy();
        M_outside_program.destroy();
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
    M_shadow_plane.create( *this, Meshes3D::ground( Sun3D::shadowLength(), Sun3D::shadowWidth() ) );
    M_far_ground.create( *this, Meshes3D::ground( 2.0f * Scenery3D::FAR_GROUND, 2.0f * Scenery3D::FAR_GROUND ) );
    // without a shadow map (a GPU that cannot draw into one), no shadows
    M_shadow_map.create( *this );
    // the realistic grass's noise, made once
    M_grass_noise.upload( *this, Grass3D::noiseImage(), true, true, false );
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
    for ( InstancedMesh * m : { &M_ground, &M_cylinder, &M_sphere, &M_cone, &M_disc, &M_ring, &M_decay_ring, &M_quad, &M_panel, &M_shadow_plane, &M_far_ground } )
    {
        m->destroy( *this );
    }
    M_shadow_map.destroy( *this );
    M_billboards.destroy( *this );
    for ( SkinnedMesh & m : M_figures ) m.destroy( *this );
    M_gpu_timer.destroy( *this );
    for ( VertexBatch * b : { &M_sector_batch, &M_line_batch, &M_dotted_batch, &M_point_batch } )
    {
        b->destroy( *this );
    }
    M_ground_texture.destroy( *this );
    M_grass_noise.destroy( *this );
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
    M_depth_program.destroy();
    M_skinned_depth_program.destroy();
    M_shadow_program.destroy();
    M_blob_program.destroy();
    M_sky_program.destroy();
    M_outside_program.destroy();
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
Scene3D::setTiming( const bool on )
{
    if ( ! M_initialized )
    {
        return;
    }
    if ( on )
    {
        M_gpu_timer.create( *this );
    }
    else
    {
        M_gpu_timer.destroy( *this );
    }
}

/*-------------------------------------------------------------------*/
void
Scene3D::setFigureBody( const int body,
                        const GltfModel & model,
                        const std::vector< FigureUniform::Paint > & paints,
                        const FigureUniform::Cuts & cuts )
{
    if ( ! M_initialized
         || body < 0
         || body >= FIGURE_BODIES )
    {
        return;
    }
    for ( int k = body; k < FIGURE_BODIES; ++k )
    {
        M_figures[k].destroy( *this );
    }
    M_figures[body].create( *this, model, paints );
    M_figure_cuts[body] = cuts;
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

    // start the frame: into the framebuffer, cleared to the horizon (the
    // sky covers it) or the background without the sky
    GLFrame::begin( *this, fbo, width, height, content.sky ? Scenery3D::SKY_HORIZON : Scenery3D::CLEAR );
    M_gpu_timer.begin( *this );

    // the figures of each body together, for the shadow map and the scene
    sortFigures( content );

    //
    // pass 0: the shadow map, the solids that cast the sun's shadows drawn
    // as the sun sees them, only their depths (Sun3D). Not the ball (its
    // own disc below tells its height) nor the cones (on the heads).
    //
    const bool shadows = ( content.shadows && M_shadow_map.created() );
    const QMatrix4x4 sun_view_proj = Sun3D::shadowViewProj();
    if ( shadows )
    {
        M_shadow_map.begin( *this );
        RenderState::opaque().apply( *this );
        M_depth_program.bind();
        M_depth_program.set( "uViewProj", sun_view_proj );
        M_cylinder.draw( *this, content.cylinders );
        M_sphere.draw( *this, content.spheres );
        if ( ! content.figures.empty() )
        {
            M_skinned_depth_program.bind();
            M_skinned_depth_program.set( "uViewProj", sun_view_proj );
            drawFigures( M_skinned_depth_program, false );
        }
        M_shadow_map.end( *this, fbo, width, height );
    }

    //
    // pass 0b: the ground beyond the ground image (in the haze with the
    // sky), a solid as the ground over it, 2 cm under it (fine for the
    // depth buffer of 24 bits: 5 mm at 200 m)
    //
    RenderState::opaque().apply( *this );
    M_background.resize( 1 );
    M_background[0].model.setToIdentity();
    M_background[0].model.translate( 0.0f, Scenery3D::FAR_GROUND_Y, 0.0f );
    M_background[0].color = content.far_ground;
    M_outside_program.bind();
    M_outside_program.set( "uViewProj", view_proj );
    M_outside_program.set( "uCamera", camera.position );
    M_outside_program.set( "uHaze", content.sky ? 1.0f : 0.0f );
    M_outside_program.set( "uHorizon", rgb( Scenery3D::SKY_HORIZON ) );
    M_outside_program.set( "uHazeFrom", Scenery3D::HAZE_FROM );
    M_outside_program.set( "uHazeTo", Scenery3D::HAZE_TO );
    M_far_ground.draw( *this, M_background );

    //
    // pass 1: the ground, with the image of the 2D view on texture unit 0
    //
    RenderState::opaque().apply( *this );
    M_ground_program.bind();
    M_ground_program.set( "uViewProj", view_proj );
    M_ground_program.set( "uTexture", 0 ); // the texture unit
    M_ground_texture.bind( *this, 0 );
    // the realistic grass over the image (Grass3D), its noise on NOISE_UNIT
    M_ground_program.set( "uRealistic", content.realistic_grass ? 1 : 0 );
    M_ground_program.set( "uNoise", NOISE_UNIT );
    M_ground_program.set( "uCamera", camera.position );
    M_ground_program.set( "uStripeWidth", float( FieldPainter::GRASS_STRIPE_WIDTH ) );
    M_ground_program.set( "uStripeContrast", Grass3D::STRIPE_CONTRAST );
    M_ground_program.set( "uStripeFixed", Grass3D::STRIPE_FIXED );
    M_ground_program.set( "uDetailTile", Grass3D::DETAIL_TILE );
    M_ground_program.set( "uDetailAmount", Grass3D::DETAIL_AMOUNT );
    M_ground_program.set( "uBladesNear", Grass3D::BLADES_NEAR );
    M_ground_program.set( "uBladesFar", Grass3D::BLADES_FAR );
    M_grass_noise.bind( *this, NOISE_UNIT );
    M_ground.draw( *this, M_ground_instance );

    //
    // pass 2: the solids, lit by the sun (the same render state)
    //
    M_lit_program.bind();
    M_lit_program.set( "uViewProj", view_proj );
    M_lit_program.set( "uSun", Sun3D::direction() );
    M_cylinder.draw( *this, content.cylinders );
    M_sphere.draw( *this, content.spheres );
    M_sphere.draw( *this, content.balls );
    M_cone.draw( *this, content.cones );

    //
    // pass 2a: the human figures, lit as the solids, their skeletons posed
    // by the skin matrices (Figures3D), each in its jersey
    //
    if ( ! content.figures.empty() )
    {
        M_skinned_program.bind();
        M_skinned_program.set( "uViewProj", view_proj );
        M_skinned_program.set( "uSun", Sun3D::direction() );
        M_skinned_program.set( "uShoeColor", FigureUniform::SHOE_COLOR );
        M_skinned_program.set( "uShortsShade", FigureUniform::SHORTS_SHADE );
        M_skinned_program.set( "uCollarSlope", FigureUniform::COLLAR_SLOPE );
        drawFigures( M_skinned_program, true );
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
    // pass 2c: the sky dome around the camera, after the solids and only
    // where none of them is (the depth test against the cleared
    // "farthest"): not over the whole screen, most of it ground
    //
    if ( content.sky )
    {
        RenderState::sky().apply( *this );
        M_background[0].model.setToIdentity();
        M_background[0].model.translate( camera.position );
        M_background[0].model.scale( Scenery3D::SKY_RADIUS );
        M_background[0].color = QVector4D( 1.0f, 1.0f, 1.0f, 1.0f );
        M_sky_program.bind();
        M_sky_program.set( "uViewProj", view_proj );
        M_sky_program.set( "uZenith", rgb( Scenery3D::SKY_ZENITH ) );
        M_sky_program.set( "uHorizon", rgb( Scenery3D::SKY_HORIZON ) );
        M_sky_program.set( "uBelow", rgb( Scenery3D::SKY_BELOW ) );
        M_sphere.draw( *this, M_background );
    }

    //
    // pass 3: the translucent ones, after the solids, which hide them where
    // nearer. They do not write the depth, so the order among them decides
    // which is over which: the layers on the ground from low to high (the
    // circles, the rings, the view areas, the lines and dots), then the nets.
    //
    RenderState::translucent().apply( *this );
    if ( shadows )
    {
        // the shadows, the lowest layer: darkened where the map has
        // something nearer to the sun, on texture unit SHADOW_UNIT
        M_shadow_program.bind();
        M_shadow_program.set( "uViewProj", view_proj );
        M_shadow_program.set( "uSunViewProj", sun_view_proj );
        M_shadow_program.set( "uShadowMap", SHADOW_UNIT );
        M_shadow_program.set( "uTexel", 1.0f / float( M_shadow_map.size() ) );
        M_shadow_program.set( "uOpacity", Sun3D::SHADOW_OPACITY );
        M_shadow_map.bind( *this, SHADOW_UNIT );
        M_shadow_plane.draw( *this, M_shadow_instance );
    }
    if ( ! content.blob_shadows.empty() )
    {
        // or the simple shadows, soft ellipses, as low
        M_blob_program.bind();
        M_blob_program.set( "uViewProj", view_proj );
        M_disc.draw( *this, content.blob_shadows );
    }
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
    M_gpu_timer.end( *this );
    GLFrame::end( *this );
}

/*-------------------------------------------------------------------*/
void
Scene3D::sortFigures( const SceneContent & content )
{
    for ( int body = 0; body < FIGURE_BODIES; ++body )
    {
        M_figure_instances[body].clear();
        M_figure_joints[body].clear();
    }
    // the skin matrices of the figures follow one another, each as many as
    // the joints of its body
    std::size_t offset = 0;
    for ( std::size_t i = 0; i < content.figures.size(); ++i )
    {
        int b = ( i < content.figure_bodies.size() ? content.figure_bodies[i] : 0 );
        if ( b < 0 || b >= FIGURE_BODIES ) b = 0;
        const std::size_t floats = std::size_t( M_figures[b].jointCount() ) * 16;
        if ( M_figures[b].created()
             && offset + floats <= content.figure_joints.size() )
        {
            const FigureInstance & f = content.figures[i];
            M_figure_instances[b].push_back( { f.color, f.look.skin,
                                               QVector4D( f.look.hair_color.toVector3D(), float( f.look.hair ) ) } );
            M_figure_joints[b].insert( M_figure_joints[b].end(),
                                       content.figure_joints.begin() + offset,
                                       content.figure_joints.begin() + offset + floats );
        }
        offset += floats;
    }
}

/*-------------------------------------------------------------------*/
void
Scene3D::drawFigures( ShaderProgram & program,
                      const bool uniform )
{
    static const char * const CUTS[FigureUniform::Cuts::COUNT] = {
        "uCuts[0]", "uCuts[1]", "uCuts[2]", "uCuts[3]", "uCuts[4]",
        "uCuts[5]", "uCuts[6]", "uCuts[7]", "uCuts[8]", "uCuts[9]", "uCuts[10]",
        "uCuts[11]", "uCuts[12]",
    };
    // each body for its figures at once, with their colors and their skin matrices
    for ( int body = 0; body < FIGURE_BODIES; ++body )
    {
        if ( M_figure_instances[body].empty() ) continue;
        if ( uniform )
        {
            // the uniform of the body (FigureUniform)
            for ( int i = 0; i < FigureUniform::Cuts::COUNT; ++i )
            {
                program.set( CUTS[i], M_figure_cuts[body].planes[i] );
            }
            const std::array< std::uint32_t, FigureUniform::SCALP_HAIRS > & scalp = M_figure_cuts[body].scalp;
            // (the bits as floats, exact to 2^24)
            program.set( "uScalp", QVector4D( float( scalp[0] ), float( scalp[1] ), float( scalp[2] ), float( scalp[3] ) ) );
        }
        M_figures[body].draw( *this, program, M_figure_instances[body], M_figure_joints[body], uniform );
    }
}
