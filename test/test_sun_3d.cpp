// -*-c++-*-

/*!
  \file test_sun_3d.cpp
  \brief unit test of Sun3D
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

#include "sun_3d.h"
#include "ground_texture.h"
#include "scene_builder.h"
#include "options.h"

#include <cmath>

/*!
  \brief test of the sun of the 3D view and the camera of its shadow map
*/
class TestSun3D
    : public QObject {
    Q_OBJECT

private slots:

    /*!
      toward the sun: of length 1, 44 degrees up, beyond the far touch line
      (the field's -y) on the left (§4.3)
    */
    void direction()
      {
          const QVector3D d = Sun3D::direction();
          QVERIFY( std::fabs( d.length() - 1.0f ) < 1.0e-5f );
          const double elevation = std::asin( d.y() ) * 180.0 / M_PI;
          QVERIFY2( std::fabs( elevation - 44.0 ) < 1.0, qPrintable( QString::number( elevation ) ) );
          QVERIFY( d.x() < 0.0f ); // the left
          QVERIFY( d.z() < 0.0f ); // the far side
      }

    /*!
      the area of the shadows: the ground image and the margins
    */
    void area()
      {
          QCOMPARE( Sun3D::shadowLength(), float( GroundTexture::length() ) + 2.0f * Sun3D::SHADOW_MARGIN );
          QCOMPARE( Sun3D::shadowWidth(), float( GroundTexture::width() ) + 2.0f * Sun3D::SHADOW_MARGIN );
      }

    /*!
      the sun's camera: the box of the shadows within OpenGL's cube, touching
      its sides (no pixel of the map wasted), along the sun's rays (a point
      and its shadow on the ground at the same place of the map), the depth
      growing away from the sun
    */
    void camera()
      {
          const QMatrix4x4 m = Sun3D::shadowViewProj();
          const float hx = Sun3D::shadowLength() * 0.5f;
          const float hz = Sun3D::shadowWidth() * 0.5f;
          float lo_x = 1.0f, hi_x = -1.0f, lo_y = 1.0f, hi_y = -1.0f;
          for ( const float x : { -hx, hx } )
          {
              for ( const float y : { 0.0f, Sun3D::SHADOW_HEIGHT } )
              {
                  for ( const float z : { -hz, hz } )
                  {
                      const QVector3D p = m.map( QVector3D( x, y, z ) );
                      QVERIFY( std::fabs( p.x() ) <= 1.0f + 1.0e-4f );
                      QVERIFY( std::fabs( p.y() ) <= 1.0f + 1.0e-4f );
                      QVERIFY( std::fabs( p.z() ) < 1.0f ); // a meter inside the near and far planes
                      lo_x = std::min( lo_x, p.x() ); hi_x = std::max( hi_x, p.x() );
                      lo_y = std::min( lo_y, p.y() ); hi_y = std::max( hi_y, p.y() );
                  }
              }
          }
          QVERIFY( std::fabs( lo_x + 1.0f ) < 1.0e-4f && std::fabs( hi_x - 1.0f ) < 1.0e-4f );
          QVERIFY( std::fabs( lo_y + 1.0f ) < 1.0e-4f && std::fabs( hi_y - 1.0f ) < 1.0e-4f );

          // a point 2 m up and where the sun's ray through it meets the ground
          const QVector3D d = Sun3D::direction();
          const QVector3D up( 10.0f, 2.0f, -5.0f );
          const QVector3D ground = up - d * ( up.y() / d.y() );
          QVERIFY( std::fabs( ground.y() ) < 1.0e-5f );
          const QVector3D a = m.map( up );
          const QVector3D b = m.map( ground );
          QVERIFY( std::fabs( a.x() - b.x() ) < 1.0e-4f );
          QVERIFY( std::fabs( a.y() - b.y() ) < 1.0e-4f );
          QVERIFY( a.z() < b.z() ); // nearer to the sun: hides the ground
          // the shadow falls away from the sun: to the right and the near side
          QVERIFY( ground.x() > up.x() && ground.z() > up.z() );
      }

    /*!
      the simple shadow: an ellipse on the plane of the shadows, from half
      the width behind the feet to half the width beyond where the sun's
      ray over the top of the head meets the ground, as wide as the body
      across
    */
    void blob()
      {
          const float height = 1.85f, width = 0.6f;
          const QMatrix4x4 m = Sun3D::blobShadow( 3.0f, -4.0f, height, width );
          const QVector3D d = Sun3D::direction();
          const QVector3D feet( 3.0f, 0.0f, -4.0f );
          const QVector3D top = QVector3D( 3.0f, height, -4.0f );
          const QVector3D shade = top - d * ( height / d.y() ); // the top's shadow
          const QVector3D away = QVector3D( shade.x() - feet.x(), 0.0f, shade.z() - feet.z() ).normalized();
          const auto near = []( const QVector3D & a, const QVector3D & b ) { return ( a - b ).length() < 1.0e-4f; };
          const QVector3D plane( 0.0f, Sun3D::SHADOW_PLANE_Y, 0.0f );

          // the ends along away, and the middle
          QVERIFY( near( m.map( QVector3D( 1.0f, 0.0f, 0.0f ) ), shade + away * ( width / 2 ) + plane ) );
          QVERIFY( near( m.map( QVector3D( -1.0f, 0.0f, 0.0f ) ), feet - away * ( width / 2 ) + plane ) );
          QVERIFY( near( m.map( QVector3D( 0.0f, 0.0f, 0.0f ) ), ( feet + shade ) * 0.5f + plane ) );
          // across: as wide as the body
          const QVector3D side = m.map( QVector3D( 0.0f, 0.0f, 1.0f ) ) - m.map( QVector3D( 0.0f, 0.0f, 0.0f ) );
          QVERIFY( std::fabs( side.length() - width / 2 ) < 1.0e-4f );
          QVERIFY( std::fabs( QVector3D::dotProduct( side, away ) ) < 1.0e-4f );
          QVERIFY( std::fabs( side.y() ) < 1.0e-6f );
          // about as long as the player is tall (the sun 44 degrees up), to the right and the near side
          QVERIFY( std::fabs( ( shade - feet ).length() - height ) < 0.1f * height );
          QVERIFY( away.x() > 0.0f && away.z() > 0.0f );
      }

    /*!
      the highest that casts a shadow is above the tallest figure (a type 1.1
      times the default, at the largest scale) with its arms raised
    */
    void height()
      {
          const float tallest = SceneBuilder::PERSON_HEIGHT * 1.1f * float( Options::MAX_FIGURE_SCALE_3D );
          QVERIFY( Sun3D::SHADOW_HEIGHT > tallest * 1.2f );
      }
};

QTEST_MAIN( TestSun3D )
#include "test_sun_3d.moc"
