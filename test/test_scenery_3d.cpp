// -*-c++-*-

/*!
  \file test_scenery_3d.cpp
  \brief unit test of Scenery3D
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

#include "scenery_3d.h"

#include <cmath>

namespace {

int
distance( const QColor & a,
          const QColor & b )
{
    return std::abs( a.red() - b.red() ) + std::abs( a.green() - b.green() ) + std::abs( a.blue() - b.blue() );
}

}

/*!
  \brief test of the scenery of the 3D view: the sky, the haze, the ground beyond
*/
class TestScenery3D
    : public QObject {
    Q_OBJECT

private slots:

    /*!
      the sky: the zenith straight up, the horizon level, lighter toward
      the horizon all the way (faster near it), and below the horizon the
      color beyond the ground from 1/12 down; any length of the direction
    */
    void sky()
      {
          using namespace Scenery3D;
          QVERIFY( distance( skyColor( QVector3D( 0, 1, 0 ) ), SKY_ZENITH ) <= 1 );
          QVERIFY( distance( skyColor( QVector3D( 0, 5, 0 ) ), SKY_ZENITH ) <= 1 );
          QVERIFY( distance( skyColor( QVector3D( 1, 0, 0 ) ), SKY_HORIZON ) <= 1 );
          QVERIFY( distance( skyColor( QVector3D( 1, -1, 0 ) ), SKY_BELOW ) <= 1 );
          QVERIFY( distance( skyColor( QVector3D( 12, -1, 0 ) ), SKY_BELOW ) <= 3 );
          // halfway below: halfway between the horizon and the color below
          const QColor half = skyColor( QVector3D( 24, -1, 0 ) );
          QVERIFY( std::abs( half.red() - ( SKY_HORIZON.red() + SKY_BELOW.red() ) / 2 ) <= 2 );
          // up from the horizon: bluer (less red) all the way, faster near the horizon
          int last = SKY_HORIZON.red() + 1;
          for ( int deg = 0; deg <= 90; deg += 10 )
          {
              const double a = deg * M_PI / 180.0;
              const int red = skyColor( QVector3D( float( std::cos( a ) ), float( std::sin( a ) ), 0.0f ) ).red();
              QVERIFY2( red < last, qPrintable( QString::number( deg ) ) );
              last = red;
          }
          const int r10 = skyColor( QVector3D( std::cos( 0.1745f ), std::sin( 0.1745f ), 0.0f ) ).red();
          QVERIFY( SKY_HORIZON.red() - r10 > ( SKY_HORIZON.red() - SKY_ZENITH.red() ) / 9 ); // more than a ninth at 10 degrees
      }

    /*!
      the haze: none out to HAZE_FROM, all of it from HAZE_TO, linear between
    */
    void haze()
      {
          using namespace Scenery3D;
          QCOMPARE( Scenery3D::haze( 0.0f ), 0.0f );
          QCOMPARE( Scenery3D::haze( HAZE_FROM ), 0.0f );
          QCOMPARE( Scenery3D::haze( ( HAZE_FROM + HAZE_TO ) / 2 ), 0.5f );
          QCOMPARE( Scenery3D::haze( HAZE_TO ), 1.0f );
          QCOMPARE( Scenery3D::haze( 3000.0f ), 1.0f );
          QVERIFY( HAZE_FROM > 200.0f ); // nothing of the stadium in the haze
          QVERIFY( FAR_GROUND >= HAZE_TO ); // the ground beyond reaches into all of the haze
          QVERIFY( SKY_RADIUS > FAR_GROUND * 1.2f );
      }

    /*!
      the ground beyond: a darker shade of the field color, the web
      version's #1a8a1a for the default field color
    */
    void farGround()
      {
          const QVector4D c = Scenery3D::farGroundColor( QColor( 0x1f, 0xa0, 0x1f ) );
          const QColor q = QColor::fromRgbF( c.x(), c.y(), c.z() );
          QVERIFY2( distance( q, QColor( 0x1a, 0x8a, 0x1a ) ) <= 3, qPrintable( q.name() ) );
          QCOMPARE( c.w(), 1.0f );
          const QVector4D white = Scenery3D::farGroundColor( Qt::white );
          QVERIFY( std::fabs( white.x() - Scenery3D::FAR_GROUND_SHADE ) < 1.0e-6f );
      }
};

QTEST_MAIN( TestScenery3D )
#include "test_scenery_3d.moc"
