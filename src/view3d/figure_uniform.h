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

#include <QStringList>
#include <QVector3D>
#include <QVector4D>

#include <array>
#include <cstdint>
#include <utility>
#include <vector>

class GltfModel;
class HumanoidRig;

/*!
  \brief the uniform of a footballer painted on a human body without
  clothes (the Universal Base Characters, docs/PLAN_3D-VIEW.md §5.8): the
  shirt with short sleeves and the socks in the figure's color, the shorts
  darker, the shoes, and the skin of the figure's color (FigureLook). No
  texture: the parts of the body come from the skin's weights, and where
  the clothes end from the skeleton at rest, so that any humanoid model can
  wear it.

  - Each vertex has how much the bones of the arms and of the legs move it
    (paintParts(); a bone without a role, such as a finger, counts for the
    role above it). A point is on an arm or a leg where that is more than
    half, else on the torso (with the head); the side by the plane between
    the shoulders (partAt()).
  - The clothes end at planes across the limbs and the body, at rest
    (cutsOf()): the sleeves across the upper arms, the shorts across the
    thighs, the socks below the knees, the shoes above the ankles, the
    collar at the neck, the shorts' waist above the hips.
  - Where a point of the body is at rest picks its color (regionAt()). The
    fragment shader of the figures (skinned.frag) does the same for every
    pixel, with the weights and the point at rest carried from the
    vertices (interpolated across a triangle), so that the edges of the
    clothes are smooth, not along the triangles.
*/
namespace FigureUniform {

//! the parts of the body (partAt())
enum class Part {
    Torso = 0, //!< with the neck and the head
    LeftArm = 1,
    RightArm = 2,
    LeftLeg = 3,
    RightLeg = 4,
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
//! the neck's width is measured this part of the way from its root to the head (above the shoulders, below the jaw)
constexpr float NECK_MEASURED = 0.7f;
//! the collar: around the neck, this many times as wide as the neck there (the median of its points' distances)
constexpr float COLLAR_WIDTH = 1.3f;
//! the collar goes down at most this far below the neck's root (m)
constexpr float COLLAR_DROP = 0.04f;
//! the collar's width grows by this much for its height above that: round, not square, where the neck meets the back
constexpr float COLLAR_SLOPE = 0.5f;
//! above this part of the way from the neck's root to the head, all is the skin (the chin, the head)
constexpr float CHIN = 0.4f;
/*!
  \brief the skin this near a hairstyle (m), the hair outward of it (within
  60 degrees of its normal: not beyond the hair's edge), is under it,
  painted in the hair's color (no scalp through a gap of the hair)
*/
constexpr float SCALP = 0.012f;
//! nearer than this (m), under it whichever way
constexpr float SCALP_TOUCH = 0.003f;
//! the hairstyles of a model whose scalps are marked at most (SkinVertex::scalp)
constexpr int SCALP_HAIRS = 4;

//! the color of the shoes (as the other colors of the 3D view, RGBA 0 to 1)
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
        COLLAR, //!< beyond (toward the head): the skin of the neck, within the collar's width
        WAIST, //!< beyond (down): the shorts
        LEFT, //!< beyond: the left side (between the shoulders)
        CHIN, //!< beyond (toward the head): the skin, of any width
        NECK, //!< not a plane: a point of the neck's middle line (xyz) and the collar's radius (w); the line along COLLAR's normal
        COUNT,
    };
    std::array< QVector4D, COUNT > planes;
    //! the hairstyle (its bit of the body: FigureBodySpec::bitOf()) of each scalp of the vertices (SkinVertex::scalp), 0: none
    std::array< std::uint32_t, SCALP_HAIRS > scalp = { { 0, 0, 0, 0 } };

    //! how far a point is from the neck's middle line (NECK), across it (m)
    float fromNeck( const QVector3D & p ) const
      {
          const QVector3D axis = planes[COLLAR].toVector3D();
          const QVector3D d = p - planes[NECK].toVector3D();
          return ( d - axis * QVector3D::dotProduct( d, axis ) ).length();
      }
    //! within the collar: the skin of the neck (wider higher up, by COLLAR_SLOPE), and all above the chin
    bool inCollar( const QVector3D & p ) const
      {
          const float h = beyond( COLLAR, p );
          return beyond( CHIN, p ) > 0.0f
              || ( h > 0.0f && fromNeck( p ) < planes[NECK].w() + COLLAR_SLOPE * h );
      }

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
  \brief how much the bones of the arms and of the legs move each vertex of
  a model (SkinVertex::part)
  \param model the model
  \param rig its roles
*/
void paintParts( GltfModel & model,
                 const HumanoidRig & rig );

/*!
  \brief the planes where the clothes end, from the model's skeleton at rest
  (and the collar from the neck of the skin)
  \param model the model, its parts painted (paintParts())
  \param rig its roles
  \param skin the materials of the skin, whose neck the collar goes round
*/
Cuts cutsOf( const GltfModel & model,
             const HumanoidRig & rig,
             const QStringList & skin );

/*!
  \brief mark the skin under each hairstyle (SkinVertex::scalp): within
  SCALP of its triangles, outward (or within SCALP_TOUCH), at rest
  \param model the model
  \param skin the materials of the skin
  \param hairs the hairstyles: their materials and bits (at most SCALP_HAIRS)
  \param cuts where the bits of the scalps are set (Cuts::scalp)
*/
void paintScalp( GltfModel & model,
                 const QStringList & skin,
                 const std::vector< std::pair< QString, std::uint32_t > > & hairs,
                 Cuts * cuts );

/*!
  \brief the part of the body of a point (as skinned.frag decides)
  \param cuts the planes (the side by Cuts::LEFT)
  \param arm how much the arms' bones move it (SkinVertex::part[0], interpolated)
  \param leg how much the legs' bones move it (SkinVertex::part[1])
  \param rest where it is at rest
*/
Part partAt( const Cuts & cuts,
             const float arm,
             const float leg,
             const QVector3D & rest );

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
  \param skin the figure's skin (FigureLook)
*/
QVector4D colorOf( const Region region,
                   const QVector4D & color,
                   const QVector4D & skin );

/*!
  \struct Paint
  \brief how a part of a figure's model (a material) is painted (SkinnedMesh)
*/
struct Paint {
    enum Kind {
        Own = 0, //!< in the material's own color
        Figure = 1, //!< in the figure's color (the mannequin's body)
        Uniform = 2, //!< in a uniform of the figure's color, with its skin (a human body)
        Hair = 3, //!< in the figure's hair color (the eyebrows and the hairstyles); a texture's shades over it
        Skin = 4, //!< in the figure's skin color; a texture's shades over it
        Tint = 5, //!< in the figure's color, a texture's shades over it (clothes of a texture)
    };
    int kind = Own;
    //! Hair: only on the figures wearing this hairstyle or extra (its bit of the body: FigureBodySpec::bitOf()); 0: on all
    std::uint32_t show = 0;
};

}

#endif
