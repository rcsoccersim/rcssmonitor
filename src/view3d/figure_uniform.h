// -*-c++-*-

/*!
  \file figure_uniform.h
  \brief the uniform painted on a human body. Header File.
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

#ifndef RCSSMONITOR_FIGURE_UNIFORM_H
#define RCSSMONITOR_FIGURE_UNIFORM_H

#include <QVector3D>
#include <QVector4D>

#include <array>

class GltfModel;
class HumanoidRig;

/*!
  \brief the uniform of a footballer painted on a human body without
  clothes (the Universal Base Characters, docs/PLAN_3D-VIEW.md §5.8): the
  shirt with short sleeves and the socks in the figure's color, the shorts
  darker, the shoes, and the skin. No texture: the parts of the body come
  from the skin's weights, and where the clothes end from the skeleton at
  rest, so that any humanoid model can wear it.

  - Each vertex is on a part of the body (paintParts()): the torso, an arm,
    a leg or the head, the part whose bones move it most (a bone without a
    role, such as a finger, counts for the role above it).
  - The clothes end at planes across the limbs and the body, at rest
    (cutsOf()): the sleeves across the upper arms, the shorts across the
    thighs, the socks below the knees, the shoes above the ankles, the
    collar at the neck, the shorts' waist above the hips.
  - Where a point of the body is at rest picks its color (regionAt()). The
    fragment shader of the figures (skinned.frag) does the same for every
    pixel, with the point at rest carried from the vertices, so that the
    edges of the clothes are straight, not along the triangles.
*/
namespace FigureUniform {

//! the parts of the body (SkinVertex::part)
enum class Part {
    Torso = 0,
    LeftArm = 1,
    RightArm = 2,
    LeftLeg = 3,
    RightLeg = 4,
    Head = 5,
};

//! what a point of the body wears
enum class Region {
    Skin,
    Shirt,
    Shorts,
    Socks,
    Shoes,
};

//! where the sleeves end: this part of the way from the shoulder to the elbow
constexpr float SLEEVE = 0.4f;
//! where the shorts end: this part of the way from the hip to the knee
constexpr float HEM = 0.5f;
//! where the socks start: this part of the way from the knee to the ankle
constexpr float SOCK = 0.12f;
//! where the shoes start: this part of the way from the ankle up to the knee
constexpr float SHOE = 0.08f;
//! where the shorts' waist is: this part of the way from the hips up to the spine
constexpr float WAIST = 0.5f;

//! the colors (as the other colors of the 3D view, RGBA 0 to 1)
const QVector4D SKIN_COLOR( 0.84f, 0.64f, 0.50f, 1.0f );
const QVector4D SHOE_COLOR( 0.07f, 0.07f, 0.08f, 1.0f );
//! the shorts are the figure's color times this
constexpr float SHORTS_SHADE = 0.4f;

/*!
  \struct Cuts
  \brief the planes where the clothes end, in the model at rest: for a
  plane (n, w) with n of length 1, dot(n, p) + w > 0 is beyond it
*/
struct Cuts {
    //! the order of the planes in planes()
    enum {
        SLEEVE_LEFT, SLEEVE_RIGHT, //!< beyond (toward the hands): the skin of the arms
        HEM_LEFT, HEM_RIGHT, //!< beyond (toward the knees): out of the shorts
        SOCK_LEFT, SOCK_RIGHT, //!< beyond (toward the feet): the socks
        SHOE_LEFT, SHOE_RIGHT, //!< beyond (toward the feet): the shoes
        COLLAR, //!< beyond (toward the head): the skin of the neck
        WAIST, //!< beyond (down): the shorts
        COUNT,
    };
    std::array< QVector4D, COUNT > planes;

    //! how far a point is beyond a plane (m; negative: before it)
    float beyond( const int plane,
                  const QVector3D & p ) const
      {
          const QVector4D & c = planes[plane];
          return c.x() * p.x() + c.y() * p.y() + c.z() * p.z() + c.w();
      }
};

/*!
  \brief the plane through a point across a direction
  \param point on the plane
  \param toward the side beyond it (any length)
*/
QVector4D planeAt( const QVector3D & point,
                   const QVector3D & toward );

/*!
  \brief the part of the body of each vertex of a model, by the weights of
  its joints (SkinVertex::part)
  \param model the model
  \param rig its roles
*/
void paintParts( GltfModel & model,
                 const HumanoidRig & rig );

//! the planes where the clothes end, from the model's skeleton at rest
Cuts cutsOf( const GltfModel & model,
             const HumanoidRig & rig );

/*!
  \brief what a point of the body wears (as skinned.frag decides)
  \param cuts the planes
  \param part the part of the body
  \param rest where the point is at rest
*/
Region regionAt( const Cuts & cuts,
                 const Part part,
                 const QVector3D & rest );

/*!
  \brief the color of a region
  \param region the region
  \param color the figure's color (the shirt)
*/
QVector4D colorOf( const Region region,
                   const QVector4D & color );

}

#endif
