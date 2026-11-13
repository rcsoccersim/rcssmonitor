// -*-c++-*-

/*!
  \file render_state.h
  \brief how the 3D view draws: depth, culling and blending. Header File.
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

#ifndef RCSSMONITOR_GL_RENDER_STATE_H
#define RCSSMONITOR_GL_RENDER_STATE_H

class QOpenGLFunctions_3_3_Core;

/*!
  \struct RenderState
  \brief how OpenGL draws the triangles of a pass.

  OpenGL keeps these settings until they are changed, so a pass that forgot
  to set one would draw with what the pass before left. Here every pass
  applies all of them (Scene3D::render()).

  - depth test: OpenGL keeps, for every pixel, how far the nearest thing
    drawn there is (the "depth buffer"). With the test, a pixel of something
    farther is not drawn: what is behind is hidden, whatever the order of
    drawing.
  - depth write: whether what is drawn writes its distance into the depth
    buffer, i.e. hides what comes after, behind it. A translucent thing must
    not hide what is behind it, so it does not write.
  - cull back faces: every triangle has a front, the side from which its
    corners go counterclockwise (Meshes3D). A closed solid never shows the
    back of its triangles, so they are skipped: half the work. A flat thing
    seen from both sides (a net) must not be culled.
  - blend: the color drawn is mixed with the color already there by its
    alpha (color.a = the opacity): result = a * new + (1 - a) * old.
*/
struct RenderState {
    bool depth_test; //!< hidden behind what is nearer
    bool depth_write; //!< hides what comes after, behind it
    bool cull_back; //!< draws the front faces only
    bool blend; //!< translucent by the alpha of the color

    //! the solids: the ground, the players, the ball, the goal posts
    static RenderState opaque() { return { true, true, true, false }; }

    //! the panels (the boards, the banners): solid, both faces (the shader paints the back)
    static RenderState opaqueTwoSided() { return { true, true, false, false }; }

    //! the layers on the ground and the nets: hidden by the solids, hiding nothing, seen from both sides
    static RenderState translucent() { return { true, false, false, true }; }

    //! the labels: over everything, translucent, seen from both sides
    static RenderState overlay() { return { false, false, false, true }; }

    //! as QPainter expects it when it draws over the scene (the HUD)
    static RenderState painter() { return { false, true, false, false }; }

    //! set them in OpenGL
    void apply( QOpenGLFunctions_3_3_Core & gl ) const;
};

#endif
