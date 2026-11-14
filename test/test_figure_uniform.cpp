// -*-c++-*-

/*!
  \file test_figure_uniform.cpp
  \brief unit test of FigureUniform
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

#include <QtTest>

#include "figure_looks.h"
#include "figure_uniform.h"
#include "figures_3d.h"

#include <cmath>
#include <limits>

using FigureUniform::Cuts;
using FigureUniform::Part;
using FigureUniform::Region;

namespace {

//! where a role's bone is at rest
QVector3D
boneAt( const Figures3D & f,
        const int body,
        const HumanBone bone )
{
    std::vector< QMatrix4x4 > global;
    f.model( body ).globals( f.model( body ).rest, global );
    return global[f.rig( body ).node( bone )].column( 3 ).toVector3D();
}

//! the vertex of the body's skin (the uniform's material) nearest a point at rest
const SkinVertex &
nearest( const Figures3D & f,
         const int body,
         const QVector3D & p )
{
    const GltfModel & m = f.model( body );
    std::size_t best = 0;
    float best_d = std::numeric_limits< float >::max();
    for ( const SkinPrimitive & prim : m.primitives )
    {
        if ( ! f.uniformMaterials( body ).contains( prim.material ) ) continue;
        for ( int i = prim.first_index; i < prim.first_index + prim.index_count; ++i )
        {
            const SkinVertex & v = m.vertices[m.indices[i]];
            const float d = ( QVector3D( v.rest[0], v.rest[1], v.rest[2] ) - p ).length();
            if ( d < best_d )
            {
                best_d = d;
                best = m.indices[i];
            }
        }
    }
    return m.vertices[best];
}

//! the highest vertex of the skin above a point (the top of a shoulder), not of an arm and below the head
const SkinVertex *
shoulderTop( const Figures3D & f,
             const int body,
             const QVector3D & p,
             const float below )
{
    const GltfModel & m = f.model( body );
    const SkinVertex * top = nullptr;
    for ( const SkinPrimitive & prim : m.primitives )
    {
        if ( ! f.uniformMaterials( body ).contains( prim.material ) ) continue;
        for ( int i = prim.first_index; i < prim.first_index + prim.index_count; ++i )
        {
            const SkinVertex & v = m.vertices[m.indices[i]];
            if ( std::fabs( v.rest[0] - p.x() ) < 0.01f
                 && std::fabs( v.rest[2] - p.z() ) < 0.03f
                 && v.rest[1] < below
                 && v.part[0] < 0.5f
                 && ( ! top || v.rest[1] > top->rest[1] ) )
            {
                top = &v;
            }
        }
    }
    return top;
}

Part
partOf( const Figures3D & f,
        const int body,
        const SkinVertex & v )
{
    return FigureUniform::partAt( f.uniformCuts( body ), v.part[0], v.part[1], QVector3D( v.rest[0], v.rest[1], v.rest[2] ) );
}

Region
regionOf( const Figures3D & f,
          const int body,
          const SkinVertex & v )
{
    return FigureUniform::regionAt( f.uniformCuts( body ), partOf( f, body, v ),
                                    QVector3D( v.rest[0], v.rest[1], v.rest[2] ) );
}

}

/*!
  \brief unit test of FigureUniform, the uniform painted on the human body
*/
class TestFigureUniform
    : public QObject {
    Q_OBJECT

    Figures3D M_figures;

private slots:

    void initTestCase()
      {
          QVERIFY2( M_figures.load(), qPrintable( M_figures.error() ) );
          QVERIFY2( M_figures.setModel( PlayerModel::Human ), qPrintable( M_figures.error() ) );
      }

    //! a plane through a point across a direction: positive beyond it, in meters
    void plane()
      {
          Cuts c;
          c.planes[Cuts::WAIST] = FigureUniform::planeAt( QVector3D( 1.0f, 2.0f, 3.0f ), QVector3D( 0.0f, -5.0f, 0.0f ) );
          QVERIFY( std::fabs( c.beyond( Cuts::WAIST, QVector3D( 7.0f, 1.5f, -2.0f ) ) - 0.5f ) < 1.0e-6f );
          QVERIFY( std::fabs( c.beyond( Cuts::WAIST, QVector3D( 0.0f, 2.25f, 0.0f ) ) + 0.25f ) < 1.0e-6f );
          QCOMPARE( c.beyond( Cuts::WAIST, QVector3D( 0.0f, 2.0f, 9.0f ) ), 0.0f );
      }

    /*!
      the part of a point: an arm or a leg where its bones move it more
      than half, the side by the plane between the shoulders; else the torso
    */
    void parts()
      {
          Cuts c;
          c.planes[Cuts::LEFT] = FigureUniform::planeAt( QVector3D( 0.0f, 1.4f, 0.0f ), QVector3D( 1.0f, 0.0f, 0.0f ) );
          using FigureUniform::partAt;
          QCOMPARE( partAt( c, 0.6f, 0.0f, QVector3D( 0.3f, 1.4f, 0.0f ) ), Part::LeftArm );
          QCOMPARE( partAt( c, 0.6f, 0.0f, QVector3D( -0.3f, 1.4f, 0.0f ) ), Part::RightArm );
          QCOMPARE( partAt( c, 0.4f, 0.0f, QVector3D( 0.3f, 1.4f, 0.0f ) ), Part::Torso );
          QCOMPARE( partAt( c, 0.0f, 0.9f, QVector3D( 0.1f, 0.5f, 0.0f ) ), Part::LeftLeg );
          QCOMPARE( partAt( c, 0.0f, 0.9f, QVector3D( -0.1f, 0.5f, 0.0f ) ), Part::RightLeg );
          QCOMPARE( partAt( c, 0.3f, 0.3f, QVector3D( 0.0f, 1.0f, 0.0f ) ), Part::Torso );
      }

    /*!
      what a point wears by its part and the planes: the sleeve and the
      skin of an arm, the shorts, the knee, the sock and the shoe of a
      leg; the waist of the torso, and the collar round the neck (wider
      higher up), all the skin above the chin
    */
    void regions()
      {
          // a body along y (up), the arms along x
          Cuts c;
          c.planes[Cuts::SLEEVE_LEFT] = FigureUniform::planeAt( QVector3D( 0.3f, 0.0f, 0.0f ), QVector3D( 1.0f, 0.0f, 0.0f ) );
          c.planes[Cuts::SLEEVE_RIGHT] = FigureUniform::planeAt( QVector3D( -0.3f, 0.0f, 0.0f ), QVector3D( -1.0f, 0.0f, 0.0f ) );
          const QVector3D down( 0.0f, -1.0f, 0.0f );
          for ( int s = 0; s < 2; ++s )
          {
              c.planes[Cuts::HEM_LEFT + s] = FigureUniform::planeAt( QVector3D( 0.0f, 0.7f, 0.0f ), down );
              c.planes[Cuts::SOCK_LEFT + s] = FigureUniform::planeAt( QVector3D( 0.0f, 0.45f, 0.0f ), down );
              c.planes[Cuts::SHOE_LEFT + s] = FigureUniform::planeAt( QVector3D( 0.0f, 0.12f, 0.0f ), down );
          }
          c.planes[Cuts::COLLAR] = FigureUniform::planeAt( QVector3D( 0.0f, 1.5f, 0.0f ), -down );
          c.planes[Cuts::CHIN] = FigureUniform::planeAt( QVector3D( 0.0f, 1.6f, 0.0f ), -down );
          c.planes[Cuts::NECK] = QVector4D( 0.0f, 1.55f, 0.02f, 0.07f ); // the neck's middle line through (0, *, 0.02), 7 cm wide
          c.planes[Cuts::WAIST] = FigureUniform::planeAt( QVector3D( 0.0f, 1.0f, 0.0f ), down );
          using FigureUniform::regionAt;
          QCOMPARE( regionAt( c, Part::LeftArm, QVector3D( 0.2f, 1.4f, 0.0f ) ), Region::Shirt );
          QCOMPARE( regionAt( c, Part::LeftArm, QVector3D( 0.5f, 1.4f, 0.0f ) ), Region::Skin );
          QCOMPARE( regionAt( c, Part::RightArm, QVector3D( -0.2f, 1.4f, 0.0f ) ), Region::Shirt );
          QCOMPARE( regionAt( c, Part::RightArm, QVector3D( -0.5f, 1.4f, 0.0f ) ), Region::Skin );
          // each arm by its own sleeve
          QCOMPARE( regionAt( c, Part::RightArm, QVector3D( 0.5f, 1.4f, 0.0f ) ), Region::Shirt );
          for ( const Part leg : { Part::LeftLeg, Part::RightLeg } )
          {
              QCOMPARE( regionAt( c, leg, QVector3D( 0.1f, 0.8f, 0.0f ) ), Region::Shorts );
              QCOMPARE( regionAt( c, leg, QVector3D( 0.1f, 0.55f, 0.0f ) ), Region::Skin );
              QCOMPARE( regionAt( c, leg, QVector3D( 0.1f, 0.3f, 0.0f ) ), Region::Socks );
              QCOMPARE( regionAt( c, leg, QVector3D( 0.1f, 0.05f, 0.1f ) ), Region::Shoes );
          }
          QCOMPARE( regionAt( c, Part::Torso, QVector3D( 0.0f, 1.3f, 0.1f ) ), Region::Shirt );
          QCOMPARE( regionAt( c, Part::Torso, QVector3D( 0.0f, 0.9f, 0.1f ) ), Region::Shorts );
          // the neck within the collar above it; the shoulders beside the neck not
          QVERIFY( std::fabs( c.fromNeck( QVector3D( 0.05f, 1.2f, 0.02f ) ) - 0.05f ) < 1.0e-6f );
          QCOMPARE( regionAt( c, Part::Torso, QVector3D( 0.0f, 1.51f, 0.08f ) ), Region::Skin );
          QCOMPARE( regionAt( c, Part::Torso, QVector3D( 0.0f, 1.49f, 0.08f ) ), Region::Shirt );
          QCOMPARE( regionAt( c, Part::Torso, QVector3D( 0.12f, 1.51f, 0.02f ) ), Region::Shirt );
          // wider higher up: 4 cm up, 2 cm wider (COLLAR_SLOPE 0.5)
          QCOMPARE( regionAt( c, Part::Torso, QVector3D( 0.085f, 1.51f, 0.02f ) ), Region::Shirt );
          QCOMPARE( regionAt( c, Part::Torso, QVector3D( 0.085f, 1.55f, 0.02f ) ), Region::Skin );
          // above the chin: the skin, however wide
          QCOMPARE( regionAt( c, Part::Torso, QVector3D( 0.3f, 1.61f, 0.0f ) ), Region::Skin );
      }

    /*!
      the skin under a hairstyle: near its triangles with the hair outward
      (or touching), not beyond the hair's edge nor far below it
    */
    void scalpSynthetic()
      {
          // a skin of four vertices on y = 0 facing up, a hair triangle 5 mm above x 0 to 1, z 0 to 1
          GltfModel m;
          auto vertex = [&]( const float x, const float y, const float z, const float ny ) {
              SkinVertex v{};
              v.position[0] = v.rest[0] = x;
              v.position[1] = v.rest[1] = y;
              v.position[2] = v.rest[2] = z;
              v.normal[1] = ny;
              v.weights[0] = 1.0f;
              m.vertices.push_back( v );
          };
          vertex( 0.2f, 0.0f, 0.2f, 1.0f );   // 0: under it
          vertex( 1.01f, 0.0f, 0.0f, 1.0f ); // 1: beyond its corner, though within SCALP of it (1.1 cm)
          vertex( 0.5f, -0.02f, 0.2f, 1.0f ); // 2: 2.5 cm below it
          vertex( 0.3f, 0.003f, 0.2f, -1.0f ); // 3: facing away, but touching it (2 mm)
          vertex( 0.0f, 0.005f, 0.0f, 1.0f ); // 4..6: the hair
          vertex( 1.0f, 0.005f, 0.0f, 1.0f );
          vertex( 0.0f, 0.005f, 2.0f, 1.0f );
          m.indices = { 0, 1, 2, 0, 2, 3, 4, 5, 6 };
          SkinPrimitive skin;
          skin.first_index = 0;
          skin.index_count = 6;
          skin.material = "Body";
          SkinPrimitive hair;
          hair.first_index = 6;
          hair.index_count = 3;
          hair.material = "Hair_Long";
          m.primitives = { skin, hair };
          Cuts c;
          FigureUniform::paintScalp( m, { "Body" }, { { "Long", 0u }, { "Hair_Long", 16u } }, &c );
          QCOMPARE( c.scalp[0], 0u );
          QCOMPARE( c.scalp[1], 16u );
          QCOMPARE( c.scalp[2], 0u );
          QCOMPARE( m.vertices[0].scalp[1], 1.0f );
          QVERIFY( ( QVector3D( 1.0f, 0.005f, 0.0f ) - QVector3D( 1.01f, 0.0f, 0.0f ) ).length() < FigureUniform::SCALP );
          QCOMPARE( m.vertices[1].scalp[1], 0.0f );
          QCOMPARE( m.vertices[2].scalp[1], 0.0f );
          QCOMPARE( m.vertices[3].scalp[1], 1.0f );
          for ( const SkinVertex & v : m.vertices ) QCOMPARE( v.scalp[0], 0.0f ); // no such material
          QCOMPARE( m.vertices[4].scalp[1], 0.0f ); // the hair itself is not the skin
      }

    /*!
      the bodies' skin under their hairstyles: the back of the head under
      each, not the face nor the nape; the bits of the scalps
    */
    void scalpBodies()
      {
          const Figures3D & f = M_figures;
          for ( int b = 0; b < 2; ++b )
          {
              const Cuts & c = f.uniformCuts( b );
              const std::uint32_t expected[2][3] = {
                  { FigureLooks::SIMPLE_PARTED, FigureLooks::BUZZED, FigureLooks::BEARD },
                  { FigureLooks::BUNS, FigureLooks::LONG, FigureLooks::BUZZED_FEMALE },
              };
              for ( int k = 0; k < 3; ++k ) QCOMPARE( c.scalp[k], expected[b][k] );
              QCOMPARE( c.scalp[3], 0u );

              const QVector3D head = boneAt( f, b, HumanBone::Head );
              const QVector3D neck = boneAt( f, b, HumanBone::Neck );
              const SkinVertex & back = nearest( f, b, head + QVector3D( 0.0f, 0.1f, -0.1f ) );
              const SkinVertex & face = nearest( f, b, head + QVector3D( 0.0f, 0.06f, 0.12f ) );
              const SkinVertex & nape = nearest( f, b, neck + ( head - neck ) * 0.3f + QVector3D( 0.0f, 0.0f, -0.06f ) );
              const SkinVertex & chin = nearest( f, b, head + QVector3D( 0.0f, -0.02f, 0.1f ) );
              const QString body = QString( " of body %1" ).arg( b );
              for ( int k = 0; k < 3; ++k )
              {
                  const bool beard = ( c.scalp[k] == FigureLooks::BEARD );
                  // the short hair close on the scalp (the long one away from it covers it itself)
                  const bool tight = ( c.scalp[k] == FigureLooks::SIMPLE_PARTED || c.scalp[k] == FigureLooks::BUZZED
                                       || c.scalp[k] == FigureLooks::BUZZED_FEMALE );
                  if ( tight ) QVERIFY2( back.scalp[k] == 1.0f, qPrintable( QString( "back of the head, hair %1" ).arg( k ) + body ) );
                  if ( beard ) QVERIFY2( back.scalp[k] == 0.0f, qPrintable( "back of the head under the beard" + body ) );
                  QVERIFY2( face.scalp[k] == 0.0f, qPrintable( QString( "face, hair %1" ).arg( k ) + body ) );
                  QVERIFY2( nape.scalp[k] == 0.0f, qPrintable( QString( "nape, hair %1" ).arg( k ) + body ) );
                  if ( beard ) QVERIFY2( chin.scalp[k] == 1.0f, qPrintable( "chin under the beard" + body ) );
              }
          }
      }

    //! the colors: the shirt and the socks of the figure, the shorts darker
    void colors()
      {
          const QVector4D red( 1.0f, 0.0f, 0.0f, 1.0f );
          const QVector4D skin( 0.5f, 0.4f, 0.3f, 1.0f );
          QCOMPARE( FigureUniform::colorOf( Region::Shirt, red, skin ), red );
          QCOMPARE( FigureUniform::colorOf( Region::Socks, red, skin ), red );
          QCOMPARE( FigureUniform::colorOf( Region::Shorts, red, skin ), QVector4D( FigureUniform::SHORTS_SHADE, 0.0f, 0.0f, 1.0f ) );
          QCOMPARE( FigureUniform::colorOf( Region::Skin, red, skin ), skin );
          QCOMPARE( FigureUniform::colorOf( Region::Shoes, red, skin ), FigureUniform::SHOE_COLOR );
      }

    /*!
      the human body: its vertices on the parts whose bones move them (the
      fingers count for the hands), and what they wear where a footballer
      wears it
    */
    void body()
      {
          const Figures3D & f = M_figures;
          for ( int b = 0; b < 2; ++b )
          {
              QCOMPARE( f.uniformMaterials( b ), QStringList( { "Body" } ) );
              QVERIFY( f.jerseyMaterials( b ).isEmpty() );

              const QVector3D hip_l = boneAt( f, b, HumanBone::LeftUpperLeg );
              const QVector3D knee_l = boneAt( f, b, HumanBone::LeftLowerLeg );
              const QVector3D ankle_l = boneAt( f, b, HumanBone::LeftFoot );
              const QVector3D toes_l = boneAt( f, b, HumanBone::LeftToes );
              const QVector3D shoulder_r = boneAt( f, b, HumanBone::RightUpperArm );
              const QVector3D shoulder_l = boneAt( f, b, HumanBone::LeftUpperArm );
              const QVector3D elbow_r = boneAt( f, b, HumanBone::RightLowerArm );
              const QVector3D hand_r = boneAt( f, b, HumanBone::RightHand );
              const QVector3D chest = boneAt( f, b, HumanBone::UpperChest );
              const QVector3D neck = boneAt( f, b, HumanBone::Neck );
              const QVector3D head = boneAt( f, b, HumanBone::Head );
              const QVector3D hips = boneAt( f, b, HumanBone::Hips );
              // the left of the model is +x
              QVERIFY( hip_l.x() > 0.0f );
              QVERIFY( shoulder_r.x() < 0.0f );

              struct Expected {
                  const char * name;
                  QVector3D at;
                  Part part;
                  Region region;
              };
              const Expected points[] = {
                  { "upper arm", shoulder_r + ( elbow_r - shoulder_r ) * 0.2f + QVector3D( 0.0f, 0.06f, 0.0f ), Part::RightArm, Region::Shirt },
                  { "forearm", elbow_r + ( hand_r - elbow_r ) * 0.5f + QVector3D( 0.0f, 0.04f, 0.0f ), Part::RightArm, Region::Skin },
                  { "finger", hand_r + ( hand_r - elbow_r ).normalized() * 0.12f, Part::RightArm, Region::Skin },
                  { "chest", chest + QVector3D( 0.0f, 0.0f, 0.15f ), Part::Torso, Region::Shirt },
                  { "back", chest + QVector3D( 0.0f, -0.1f, -0.15f ), Part::Torso, Region::Shirt },
                  { "belly", hips + QVector3D( 0.0f, 0.2f, 0.15f ), Part::Torso, Region::Shirt },
                  { "thigh", hip_l + ( knee_l - hip_l ) * 0.25f + QVector3D( 0.0f, 0.0f, 0.1f ), Part::LeftLeg, Region::Shorts },
                  { "knee", knee_l + QVector3D( 0.0f, 0.0f, 0.07f ), Part::LeftLeg, Region::Skin },
                  { "shin", knee_l + ( ankle_l - knee_l ) * 0.5f + QVector3D( 0.0f, 0.0f, 0.06f ), Part::LeftLeg, Region::Socks },
                  { "toes", toes_l + QVector3D( 0.0f, 0.03f, 0.04f ), Part::LeftLeg, Region::Shoes },
                  // the neck within the collar
                  { "front of the neck", neck + ( head - neck ) * 0.5f + QVector3D( 0.0f, 0.0f, 0.1f ), Part::Torso, Region::Skin },
                  { "back of the neck", neck + ( head - neck ) * 0.5f + QVector3D( 0.0f, 0.0f, -0.03f ), Part::Torso, Region::Skin },
                  { "face", head + QVector3D( 0.0f, 0.08f, 0.12f ), Part::Torso, Region::Skin },
                  { "top of the head", head + QVector3D( 0.0f, 0.2f, 0.0f ), Part::Torso, Region::Skin },
              };
              for ( const Expected & e : points )
              {
                  const SkinVertex & v = nearest( f, b, e.at );
                  const QString what = QString( "%1 of body %2" ).arg( e.name ).arg( b );
                  QVERIFY2( ( QVector3D( v.rest[0], v.rest[1], v.rest[2] ) - e.at ).length() < 0.1f, qPrintable( what ) );
                  QVERIFY2( partOf( f, b, v ) == e.part, qPrintable( what + QString( ": part %1" ).arg( int( partOf( f, b, v ) ) ) ) );
                  QVERIFY2( regionOf( f, b, v ) == e.region, qPrintable( what + QString( ": region %1" ).arg( int( regionOf( f, b, v ) ) ) ) );
              }

              // the shoulders' tops beside the neck in the shirt, not the skin
              // of the neck (2026-11-14, user: torn from the neck to the shoulders)
              for ( const QVector3D & shoulder : { shoulder_l, shoulder_r } )
              {
                  for ( const float t : { 0.6f, 0.7f, 0.8f } )
                  {
                      const SkinVertex * top = shoulderTop( f, b, neck + ( shoulder - neck ) * t, head.y() );
                      QVERIFY( top );
                      const QString what = QString( "the shoulder's top at %1 of body %2 (%3 m high)" ).arg( t ).arg( b ).arg( top->rest[1] );
                      QVERIFY2( partOf( f, b, *top ) == Part::Torso, qPrintable( what ) );
                      QVERIFY2( regionOf( f, b, *top ) == Region::Shirt, qPrintable( what ) );
                  }
              }

              // the planes in order down a leg, and the collar at the neck below the head
              const Cuts & c = f.uniformCuts( b );
              QVERIFY( c.beyond( Cuts::HEM_LEFT, hip_l ) < 0.0f );
              QVERIFY( c.beyond( Cuts::HEM_LEFT, knee_l ) > 0.0f );
              QVERIFY( c.beyond( Cuts::SOCK_LEFT, knee_l ) < 0.0f );
              QVERIFY( c.beyond( Cuts::SOCK_LEFT, ankle_l ) > 0.0f );
              QVERIFY( c.beyond( Cuts::SHOE_LEFT, ankle_l ) > 0.0f );
              QVERIFY( c.beyond( Cuts::SHOE_LEFT, knee_l ) < 0.0f );
              QVERIFY( c.beyond( Cuts::SLEEVE_RIGHT, shoulder_r ) < 0.0f );
              QVERIFY( c.beyond( Cuts::SLEEVE_RIGHT, elbow_r ) > 0.0f );
              QVERIFY( c.beyond( Cuts::SLEEVE_LEFT, elbow_r ) < 0.0f ); // the other arm's sleeve
              QVERIFY( c.beyond( Cuts::LEFT, shoulder_l ) > 0.0f );
              QVERIFY( c.beyond( Cuts::LEFT, shoulder_r ) < 0.0f );
              QVERIFY( c.beyond( Cuts::COLLAR, head ) > 0.0f );
              QVERIFY( c.beyond( Cuts::COLLAR, chest ) < 0.0f );
              QVERIFY( c.beyond( Cuts::CHIN, head ) > 0.0f );
              QVERIFY( c.beyond( Cuts::CHIN, neck ) < 0.0f );
              QVERIFY( c.beyond( Cuts::WAIST, hips ) > 0.0f );
              QVERIFY( c.beyond( Cuts::WAIST, chest ) < 0.0f );
              // the collar about as wide as a neck, round its middle (in front of the bones)
              const float width = c.planes[Cuts::NECK].w();
              QVERIFY2( width > 0.04f && width < 0.12f, qPrintable( QString::number( width ) ) );
              QVERIFY( c.planes[Cuts::NECK].z() > neck.z() );
          }
      }
};

QTEST_MAIN( TestFigureUniform )
#include "test_figure_uniform.moc"
