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
        const HumanBone bone )
{
    std::vector< QMatrix4x4 > global;
    f.model().globals( f.model().rest, global );
    return global[f.rig().node( bone )].column( 3 ).toVector3D();
}

//! the vertex of the body's skin (the uniform's material) nearest a point at rest
const SkinVertex &
nearest( const Figures3D & f,
         const QVector3D & p )
{
    const GltfModel & m = f.model();
    std::size_t best = 0;
    float best_d = std::numeric_limits< float >::max();
    for ( const SkinPrimitive & prim : m.primitives )
    {
        if ( ! f.uniformMaterials().contains( prim.material ) ) continue;
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

Region
regionOf( const Figures3D & f,
          const SkinVertex & v )
{
    return FigureUniform::regionAt( f.uniformCuts(), static_cast< Part >( int( v.part ) ),
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
      what a point wears by its part and the planes: the sleeve and the
      skin of an arm, the shorts, the knee, the sock and the shoe of a
      leg, the collar and the waist of the torso, the head's skin
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
          QCOMPARE( regionAt( c, Part::Torso, QVector3D( 0.0f, 1.6f, 0.0f ) ), Region::Skin );
          QCOMPARE( regionAt( c, Part::Torso, QVector3D( 0.0f, 1.3f, 0.1f ) ), Region::Shirt );
          QCOMPARE( regionAt( c, Part::Torso, QVector3D( 0.0f, 0.9f, 0.1f ) ), Region::Shorts );
          QCOMPARE( regionAt( c, Part::Head, QVector3D( 0.0f, 1.2f, 0.0f ) ), Region::Skin );
      }

    //! the colors: the shirt and the socks of the figure, the shorts darker
    void colors()
      {
          const QVector4D red( 1.0f, 0.0f, 0.0f, 1.0f );
          QCOMPARE( FigureUniform::colorOf( Region::Shirt, red ), red );
          QCOMPARE( FigureUniform::colorOf( Region::Socks, red ), red );
          QCOMPARE( FigureUniform::colorOf( Region::Shorts, red ), QVector4D( FigureUniform::SHORTS_SHADE, 0.0f, 0.0f, 1.0f ) );
          QCOMPARE( FigureUniform::colorOf( Region::Skin, red ), FigureUniform::SKIN_COLOR );
          QCOMPARE( FigureUniform::colorOf( Region::Shoes, red ), FigureUniform::SHOE_COLOR );
      }

    /*!
      the human body: its vertices on the parts whose bones move them (the
      fingers count for the hands), and what they wear where a footballer
      wears it
    */
    void body()
      {
          const Figures3D & f = M_figures;
          QCOMPARE( f.uniformMaterials(), QStringList( { "MI_Superhero_Male" } ) );
          QVERIFY( f.jerseyMaterials().isEmpty() );

          const QVector3D hip_l = boneAt( f, HumanBone::LeftUpperLeg );
          const QVector3D knee_l = boneAt( f, HumanBone::LeftLowerLeg );
          const QVector3D ankle_l = boneAt( f, HumanBone::LeftFoot );
          const QVector3D toes_l = boneAt( f, HumanBone::LeftToes );
          const QVector3D shoulder_r = boneAt( f, HumanBone::RightUpperArm );
          const QVector3D elbow_r = boneAt( f, HumanBone::RightLowerArm );
          const QVector3D hand_r = boneAt( f, HumanBone::RightHand );
          const QVector3D chest = boneAt( f, HumanBone::UpperChest );
          const QVector3D head = boneAt( f, HumanBone::Head );
          const QVector3D hips = boneAt( f, HumanBone::Hips );
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
              { "face", head + QVector3D( 0.0f, 0.08f, 0.12f ), Part::Head, Region::Skin },
              { "top of the head", head + QVector3D( 0.0f, 0.2f, 0.0f ), Part::Head, Region::Skin },
          };
          for ( const Expected & e : points )
          {
              const SkinVertex & v = nearest( f, e.at );
              QVERIFY2( ( QVector3D( v.rest[0], v.rest[1], v.rest[2] ) - e.at ).length() < 0.1f, e.name );
              QVERIFY2( int( v.part ) == int( e.part ), qPrintable( QString( "%1: part %2" ).arg( e.name ).arg( v.part ) ) );
              QVERIFY2( regionOf( f, v ) == e.region, qPrintable( QString( "%1: region %2" ).arg( e.name ).arg( int( regionOf( f, v ) ) ) ) );
          }

          // the planes in order down a leg, and the collar at the neck below the head
          const Cuts & c = f.uniformCuts();
          QVERIFY( c.beyond( Cuts::HEM_LEFT, hip_l ) < 0.0f );
          QVERIFY( c.beyond( Cuts::HEM_LEFT, knee_l ) > 0.0f );
          QVERIFY( c.beyond( Cuts::SOCK_LEFT, knee_l ) < 0.0f );
          QVERIFY( c.beyond( Cuts::SOCK_LEFT, ankle_l ) > 0.0f );
          QVERIFY( c.beyond( Cuts::SHOE_LEFT, ankle_l ) > 0.0f );
          QVERIFY( c.beyond( Cuts::SHOE_LEFT, knee_l ) < 0.0f );
          QVERIFY( c.beyond( Cuts::SLEEVE_RIGHT, shoulder_r ) < 0.0f );
          QVERIFY( c.beyond( Cuts::SLEEVE_RIGHT, elbow_r ) > 0.0f );
          QVERIFY( c.beyond( Cuts::SLEEVE_LEFT, elbow_r ) < 0.0f ); // the other arm's sleeve
          QVERIFY( c.beyond( Cuts::COLLAR, head ) > 0.0f );
          QVERIFY( c.beyond( Cuts::COLLAR, chest ) < 0.0f );
          QVERIFY( c.beyond( Cuts::WAIST, hips ) > 0.0f );
          QVERIFY( c.beyond( Cuts::WAIST, chest ) < 0.0f );
      }
};

QTEST_MAIN( TestFigureUniform )
#include "test_figure_uniform.moc"
