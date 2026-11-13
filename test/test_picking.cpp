// -*-c++-*-

/*!
  \file test_picking.cpp
  \brief unit test of what a click on the 3D view aims at
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

#include "picking.h"

#include "ground_texture.h"
#include "scene_builder.h"

#include <QMatrix4x4>
#include <QVector4D>

#include <cmath>

namespace {

const QSizeF VIEW( 800.0, 450.0 );

//! a camera of the broadcast kind, from the side of the pitch
CameraPose
sideCamera()
{
    CameraPose c;
    c.position = QVector3D( 0.0f, 30.0f, 60.0f );
    c.target = QVector3D( 0.0f, 0.0f, 0.0f );
    return c;
}

//! the top camera: orthographic, looking straight down, -z up on the screen
CameraPose
topCamera()
{
    CameraPose c;
    c.position = QVector3D( 0.0f, 100.0f, 0.0f );
    c.target = QVector3D( 0.0f, 0.0f, 0.0f );
    c.up = QVector3D( 0.0f, 0.0f, -1.0f );
    c.ortho_half_height = 40.0f;
    return c;
}

//! where a point of the world is on the screen (logical pixels)
QPointF
project( const CameraPose & camera,
         const QVector3D & world )
{
    const QMatrix4x4 m = camera.projection( float( VIEW.width() / VIEW.height() ) ) * camera.view();
    const QVector4D clip = m.map( QVector4D( world, 1.0f ) );
    const QVector3D ndc = clip.toVector3D() / clip.w();
    return QPointF( ( ndc.x() + 1.0 ) / 2.0 * VIEW.width(),
                    ( 1.0 - ndc.y() ) / 2.0 * VIEW.height() );
}

//! how far a point is from a ray
float
distance( const Picking::Ray & ray,
          const QVector3D & p )
{
    return QVector3D::crossProduct( p - ray.origin, ray.direction ).length();
}

PlayerPick
pick( const rcss::rcg::Side side,
      const int unum,
      const float x,
      const float y )
{
    return PlayerPick{ side, unum, x, y, 0.3f };
}

}

/*!
  \brief unit test of Picking
*/
class TestPicking
    : public QObject {
    Q_OBJECT

private slots:

    /*!
      The ray through a point of the screen passes through the points of
      the world drawn there, from the camera, for the perspective and the
      orthographic cameras.
    */
    void ray()
      {
          for ( const CameraPose & camera : { sideCamera(), topCamera() } )
          {
              // through the center: toward what the camera looks at
              const Picking::Ray center = Picking::rayAt( camera, VIEW, QPointF( VIEW.width() / 2, VIEW.height() / 2 ) );
              QVERIFY( qFuzzyCompare( center.direction.length(), 1.0f ) );
              QVERIFY( ( center.direction - ( camera.target - camera.position ).normalized() ).length() < 1.0e-4f );
              QVERIFY( distance( center, camera.target ) < 1.0e-3f );

              for ( const QVector3D & p : { QVector3D( 20.0f, 0.0f, -10.0f ),
                                            QVector3D( -30.0f, 1.5f, 15.0f ),
                                            QVector3D( 5.0f, 0.7f, 25.0f ) } )
              {
                  const Picking::Ray ray = Picking::rayAt( camera, VIEW, project( camera, p ) );
                  QVERIFY2( distance( ray, p ) < 1.0e-3f, qPrintable( QString::number( distance( ray, p ) ) ) );
                  // from the camera's side, toward the point
                  QVERIFY( QVector3D::dotProduct( p - ray.origin, ray.direction ) > 0.0f );
              }
          }

          // the top camera's rays are parallel, straight down
          const Picking::Ray corner = Picking::rayAt( topCamera(), VIEW, QPointF( 0, 0 ) );
          QVERIFY( ( corner.direction - QVector3D( 0, -1, 0 ) ).length() < 1.0e-4f );
          // the top left corner is at -x and -y of the field (-y up, as in 2D)
          QVERIFY( corner.origin.x() < -60.0f );
          QVERIFY( corner.origin.z() < -35.0f );
      }

    void ground()
      {
          const CameraPose camera = sideCamera();
          QVector2D field;
          QVERIFY( Picking::groundAt( Picking::rayAt( camera, VIEW, project( camera, QVector3D( 20, 0, -10 ) ) ), &field ) );
          QVERIFY( ( field - QVector2D( 20, -10 ) ).length() < 1.0e-3f );

          // up or along the ground: above the horizon
          Picking::Ray up{ QVector3D( 0, 10, 0 ), QVector3D( 1, 0.1f, 0 ).normalized() };
          QVERIFY( ! Picking::groundAt( up, &field ) );
          Picking::Ray level{ QVector3D( 0, 10, 0 ), QVector3D( 1, 0, 0 ) };
          QVERIFY( ! Picking::groundAt( level, &field ) );
          // the side camera looks down 26.6 degrees, with 17.5 above the
          // center of the screen: even the top of the screen meets the ground
          QVERIFY( Picking::groundAt( Picking::rayAt( camera, VIEW, QPointF( 400, 0 ) ), &field ) );
      }

    /*!
      The body (a cylinder standing on the ground) and the head (a sphere)
      of a player are hit where they are drawn, and not beside them.
    */
    void player()
      {
          const CameraPose camera = sideCamera();
          const std::vector< PlayerPick > picks = { pick( rcss::rcg::LEFT, 4, 10.0f, 5.0f ) };

          for ( const float height : { 0.1f, 0.7f, 1.35f } )
          {
              const QPointF at = project( camera, QVector3D( 10.0f, height, 5.0f ) );
              QCOMPARE( Picking::playerAt( Picking::rayAt( camera, VIEW, at ), picks ), 0 );
          }
          // the head
          QCOMPARE( Picking::playerAt( Picking::rayAt( camera, VIEW, project( camera, QVector3D( 10.0f, SceneBuilder::HEAD_HEIGHT + 0.15f, 5.0f ) ) ), picks ), 0 );
          // above the head, beside the body, and the ground in front of it
          QCOMPARE( Picking::playerAt( Picking::rayAt( camera, VIEW, project( camera, QVector3D( 10.0f, SceneBuilder::HEAD_HEIGHT + 0.3f, 5.0f ) ) ), picks ), -1 );
          QCOMPARE( Picking::playerAt( Picking::rayAt( camera, VIEW, project( camera, QVector3D( 10.5f, 0.7f, 5.0f ) ) ), picks ), -1 );
          QCOMPARE( Picking::playerAt( Picking::rayAt( camera, VIEW, project( camera, QVector3D( 10.0f, 0.0f, 5.5f ) ) ), picks ), -1 );
          // none
          QCOMPARE( Picking::playerAt( Picking::rayAt( camera, VIEW, QPointF( 400, 225 ) ), std::vector< PlayerPick >() ), -1 );
      }

    //! the nearest of the players on the ray, whatever their order
    void nearest()
      {
          const CameraPose camera = sideCamera();
          // two players on the line of sight to the body of one at the
          // center: the other in front, where the line is 1 m high
          const QVector3D far( 0.0f, 0.7f, 0.0f );
          const QVector3D toward = ( camera.position - far ).normalized();
          const QVector3D front = far + toward * ( ( 1.0f - far.y() ) / toward.y() );
          const QPointF at = project( camera, far );
          const std::vector< PlayerPick > picks = {
              pick( rcss::rcg::LEFT, 2, far.x(), far.z() ),
              pick( rcss::rcg::RIGHT, 9, front.x(), front.z() ),
          };
          const int i = Picking::playerAt( Picking::rayAt( camera, VIEW, at ), picks );
          QCOMPARE( i, 1 );
          const std::vector< PlayerPick > reversed = { picks[1], picks[0] };
          QCOMPARE( Picking::playerAt( Picking::rayAt( camera, VIEW, at ), reversed ), 0 );
      }

    //! from the top camera, straight down: the top of the body and the head
    void fromAbove()
      {
          const CameraPose camera = topCamera();
          const std::vector< PlayerPick > picks = { pick( rcss::rcg::RIGHT, 7, -20.0f, 10.0f ) };
          QCOMPARE( Picking::playerAt( Picking::rayAt( camera, VIEW, project( camera, QVector3D( -20.0f, 0.0f, 10.0f ) ) ), picks ), 0 );
          // within the body (0.3) but outside the head (0.22)
          QCOMPARE( Picking::playerAt( Picking::rayAt( camera, VIEW, project( camera, QVector3D( -20.25f, 0.0f, 10.0f ) ) ), picks ), 0 );
          // outside the body
          QCOMPARE( Picking::playerAt( Picking::rayAt( camera, VIEW, project( camera, QVector3D( -20.4f, 0.0f, 10.0f ) ) ), picks ), -1 );
      }

    /*!
      A right click aims at the player under it, at its feet; else at the
      ground on the pitch or its margin; else at nothing.
    */
    void target()
      {
          const CameraPose camera = sideCamera();
          const std::vector< PlayerPick > picks = { pick( rcss::rcg::RIGHT, 7, 10.0f, 5.0f ) };

          RefereeTarget t = Picking::targetAt( camera, VIEW, project( camera, QVector3D( 10.0f, 1.0f, 5.0f ) ), picks );
          QVERIFY( t.has_point_ );
          QVERIFY( t.hasPlayer() );
          QCOMPARE( t.side_, rcss::rcg::RIGHT );
          QCOMPARE( t.unum_, 7 );
          QCOMPARE( t.x_, 10.0 );
          QCOMPARE( t.y_, 5.0 );

          // the ground in the margin
          const float edge_x = float( GroundTexture::length() / 2 ) - 1.0f;
          t = Picking::targetAt( camera, VIEW, project( camera, QVector3D( -edge_x, 0.0f, -20.0f ) ), picks );
          QVERIFY( t.has_point_ );
          QVERIFY( ! t.hasPlayer() );
          QVERIFY( std::fabs( t.x_ + edge_x ) < 1.0e-3 );
          QVERIFY( std::fabs( t.y_ + 20.0 ) < 1.0e-3 );

          // beyond the margin
          t = Picking::targetAt( camera, VIEW, project( camera, QVector3D( -edge_x - 2.0f, 0.0f, -20.0f ) ), picks );
          QVERIFY( ! t.has_point_ );
          QVERIFY( ! t.hasPlayer() );
          const float edge_y = float( GroundTexture::width() / 2 ) + 1.0f;
          t = Picking::targetAt( camera, VIEW, project( camera, QVector3D( 0.0f, 0.0f, -edge_y ) ), picks );
          QVERIFY( ! t.has_point_ );

          // above the horizon: a camera looking along the ground
          CameraPose low;
          low.position = QVector3D( 0.0f, 2.0f, 30.0f );
          low.target = QVector3D( 0.0f, 2.0f, 0.0f );
          t = Picking::targetAt( low, VIEW, QPointF( 400, 100 ), picks );
          QVERIFY( ! t.has_point_ );
          // and below it, near
          t = Picking::targetAt( low, VIEW, QPointF( 400, 440 ), picks );
          QVERIFY( t.has_point_ );
      }
};

QTEST_MAIN( TestPicking )
#include "test_picking.moc"
