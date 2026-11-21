// -*-c++-*-

/*!
  \file skinned_mesh.h
  \brief a skinned mesh (a human figure), drawn in instances. Header File.
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

#ifndef RCSSMONITOR_GL_SKINNED_MESH_H
#define RCSSMONITOR_GL_SKINNED_MESH_H

#include "figure_uniform.h"
#include "gltf_model.h"
#include "shader_program.h"
#include "texture_2d.h"

#include <QOpenGLFunctions_3_3_Core>
#include <QVector4D>

#include <memory>
#include <vector>

/*!
  \class SkinnedMesh
  \brief a skinned mesh, the human figure of a GltfModel, and its copies
  (the players) drawn all at once.

  A skinned vertex moves with up to 4 joints (bones) of a skeleton, by
  their weights: the vertex shader puts it at the sum of weight * the
  joint's "skin matrix" * the vertex. The skin matrices of a figure
  (Figures3D::pose(): where its joints are in the world in its pose) differ
  for every figure in every frame. They go to the GPU in a texture of
  floats, not in a buffer of attributes: a row a figure, 4 texels (the 4
  columns of the matrix) a joint. The shader reads the row of its instance
  (gl_InstanceID) with texelFetch(), which reads a texel by its integer
  place, not filtered.

  The mesh has parts in materials (SkinPrimitive), each painted as told
  (FigureUniform::Paint): in each figure's color (an attribute of the
  instance); a body in a uniform of that color with the figure's skin
  (FigureUniform: the fragment shader paints it by the part of the body
  and where the point is at rest, attributes of the vertex); the eyebrows
  and the hairstyles in the figure's hair color, a hairstyle only on the
  figures wearing it (the vertex shader puts it out of sight on the
  others); and the others in their own color. Each part is one instanced
  draw call for all the figures.

  A material with a texture (the model's images, sent once): in its own
  color, the texture times it; in the hair's, the skin's or the figure's
  color (Tint), the texture's shades over that color (its luminance, of
  its mean); in the figure's color (Figure) and in the uniform, flat, but
  for the texture's transparent parts. A part transparent in places (a
  mask, or a blend, drawn as a mask) is not drawn there, in the shadow map
  either. A double-sided part is drawn from behind too.

  Every function needs the OpenGL context current.
*/
class SkinnedMesh {
public:

    /*!
      \struct Instance
      \brief a figure's colors, the attributes of its instance
    */
    struct Instance {
        QVector4D color; //!< the figure's color (COLOR)
        QVector4D skin; //!< its skin (SKIN)
        QVector4D hair; //!< its hair, with the hairstyles it wears in w (HAIR)
    };

    SkinnedMesh() = default;
    SkinnedMesh( const SkinnedMesh & ) = delete;
    SkinnedMesh & operator=( const SkinnedMesh & ) = delete;

    /*!
      \brief send the mesh to the GPU
      \param gl the OpenGL functions
      \param model the model: its vertices, triangles and parts
      \param paints how each part (model.primitives) is painted (missing: in its own color)
    */
    void create( QOpenGLFunctions_3_3_Core & gl,
                 const GltfModel & model,
                 const std::vector< FigureUniform::Paint > & paints );

    //! delete the buffers, the VAO and the texture
    void destroy( QOpenGLFunctions_3_3_Core & gl );

    bool created() const { return M_vao != 0; }

    int jointCount() const { return M_joint_count; }

    /*!
      \brief send the figures of a frame to the GPU: their colors and their
      skin matrices (for draw(), in the shadow map and in the scene)
      \param gl the OpenGL functions
      \param figures each figure's colors
      \param joints the skin matrices: jointCount() * 16 floats a figure, as many figures
    */
    void setFigures( QOpenGLFunctions_3_3_Core & gl,
                     const std::vector< Instance > & figures,
                     const std::vector< float > & joints );

    //! whether a part has a texture or a transparency (drawn with the TEXTURED shaders)
    bool hasTextured() const { return M_textured; }

    /*!
      \brief draw the figures of setFigures(): the plain parts, or those
      with a texture or a transparency (whose shaders are another variant,
      so that the plain ones stay as light), with the shader program that
      is bound (the "skinned" vertex shader) and the render state that is
      applied
      \param gl the OpenGL functions
      \param program the bound program: draw() sets its uniforms uJoints
      (the texture unit of the skin matrices), uPaint (FigureUniform::Paint::Kind),
      uShow (the hairstyle of a part), uBaseColor, and those of a texture
      (uTextured, uTexture, uAlphaCutoff, uLumScale); the caller sets those
      of the uniform
      \param textured the parts with a texture or a transparency, else the plain ones
      \param colors false for the shadow map, which uses no colors:
      uBaseColor is not set (the compiler removes it)
    */
    void draw( QOpenGLFunctions_3_3_Core & gl,
               ShaderProgram & program,
               const bool textured,
               const bool colors = true );

private:
    struct Part {
        int first_index;
        int index_count;
        int paint; //!< uPaint
        int show; //!< uShow
        QVector4D color;
        int texture; //!< of M_textures, -1: none
        float alpha_cutoff; //!< not drawn where less transparent; < 0: opaque
        float lum_scale; //!< a texture's luminance times this is its shade (1 / its mean)
        bool two_sided; //!< drawn from behind too
        bool textured() const { return texture >= 0 || alpha_cutoff >= 0.0f; }
    };

    GLuint M_vao = 0;
    GLuint M_vertex_buffer = 0;
    GLuint M_uv_buffer = 0; //!< GltfModel::uvs
    GLuint M_index_buffer = 0;
    GLuint M_instance_buffer = 0; //!< the colors of the figures (Instance)
    GLuint M_joint_texture = 0; //!< the skin matrices, a row a figure
    std::vector< std::unique_ptr< Texture2D > > M_textures; //!< the model's images
    int M_joint_count = 0;
    std::size_t M_capacity = 0; //!< the figures that the instance buffer and the texture have room for
    GLsizei M_count = 0; //!< the figures of setFigures()
    bool M_textured = false; //!< hasTextured()
    std::vector< Part > M_parts;
};

#endif
