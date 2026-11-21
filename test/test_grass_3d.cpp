// -*-c++-*-

/*!
  \file test_grass_3d.cpp
  \brief unit test of Grass3D
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

#include "grass_3d.h"
#include "field_painter.h"

#include <cmath>

/*!
  \brief test of the realistic grass of the 3D view
*/
class TestGrass3D
    : public QObject {
    Q_OBJECT

private slots:

    /*!
      the noise: gray, about 0.5 with a spread, the same for the same seed,
      and tiling (across its edges as smooth as inside)
    */
    void noise()
      {
          const QImage a = Grass3D::noiseImage();
          QCOMPARE( a.size(), QSize( Grass3D::NOISE_SIZE, Grass3D::NOISE_SIZE ) );
          QCOMPARE( a.format(), QImage::Format_Grayscale8 );
          QCOMPARE( Grass3D::noiseImage(), a );
          QVERIFY( Grass3D::noiseImage( Grass3D::NOISE_SIZE, 7u ) != a );

          const int n = a.width();
          const auto v = [&]( int x, int y ) { return double( a.constScanLine( y )[x] ) / 255.0; };
          double mean = 0.0, sq = 0.0;
          double inside = 0.0, across = 0.0, inside_down = 0.0, across_down = 0.0;
          for ( int y = 0; y < n; ++y )
          {
              for ( int x = 0; x < n; ++x )
              {
                  mean += v( x, y );
                  sq += v( x, y ) * v( x, y );
              }
              for ( int x = 0; x + 1 < n; ++x ) inside += std::fabs( v( x, y ) - v( x + 1, y ) );
              across += std::fabs( v( n - 1, y ) - v( 0, y ) );
          }
          for ( int x = 0; x < n; ++x )
          {
              for ( int y = 0; y + 1 < n; ++y ) inside_down += std::fabs( v( x, y ) - v( x, y + 1 ) );
              across_down += std::fabs( v( x, n - 1 ) - v( x, 0 ) );
          }
          mean /= double( n ) * n;
          const double sd = std::sqrt( sq / ( double( n ) * n ) - mean * mean );
          QVERIFY2( std::fabs( mean - 0.5 ) < 0.05, qPrintable( QString::number( mean ) ) );
          QVERIFY2( sd > 0.1 && sd < 0.3, qPrintable( QString::number( sd ) ) );
          inside /= double( n ) * ( n - 1 );
          across /= double( n );
          inside_down /= double( n ) * ( n - 1 );
          across_down /= double( n );
          QVERIFY2( across < 2.0 * inside, qPrintable( QString( "%1 %2" ).arg( across ).arg( inside ) ) );
          QVERIFY2( across_down < 2.0 * inside_down, qPrintable( QString( "%1 %2" ).arg( across_down ).arg( inside_down ) ) );
      }

    /*!
      the mowed stripes: from the main cameras' side, the even stripes light
      and the odd ones dark (on both sides of the halfway line); from the
      other side, swapped; from straight above, only the fixed part; within
      the contrast
    */
    void stripes()
      {
          using Grass3D::stripeShade;
          const float w = float( FieldPainter::GRASS_STRIPE_WIDTH );
          const QVector3D near( 0.0f, 25.0f, 70.0f ), far( 0.0f, 25.0f, -70.0f );
          for ( const float x : { 0.5f * w, 2.5f * w, -1.5f * w } ) // even stripes: k = 0, 2, -2
          {
              QVERIFY( stripeShade( x, -10.0f, near ) > 1.03f );
              QVERIFY( stripeShade( x + w, -10.0f, near ) < 0.97f ); // odd: k = 1, 3, -1
              // swapped, but fainter: the fixed part stays
              QVERIFY( stripeShade( x, 10.0f, far ) < 0.99f );
              QVERIFY( stripeShade( x + w, 10.0f, far ) > 1.01f );
          }
          // straight above: the fixed part only
          const float fixed = Grass3D::STRIPE_CONTRAST * Grass3D::STRIPE_FIXED;
          QVERIFY( std::fabs( stripeShade( 0.5f * w, 0.0f, QVector3D( 0.5f * w, 50.0f, 0.0f ) ) - ( 1.0f + fixed ) ) < 1.0e-5f );
          QVERIFY( std::fabs( stripeShade( 1.5f * w, 0.0f, QVector3D( 1.5f * w, 50.0f, 0.0f ) ) - ( 1.0f - fixed ) ) < 1.0e-5f );
          // seen across (along x), the fixed part only
          QVERIFY( std::fabs( stripeShade( 0.5f * w, 0.0f, QVector3D( -80.0f, 0.0f, 0.0f ) ) - ( 1.0f + fixed ) ) < 1.0e-5f );
          // within the contrast, from anywhere
          for ( float cx = -60.0f; cx <= 60.0f; cx += 30.0f )
          {
              for ( float x = -50.0f; x <= 50.0f; x += 3.7f )
              {
                  const float s = stripeShade( x, 7.0f, QVector3D( cx, 1.0f, -cx ) );
                  QVERIFY( s >= 1.0f - Grass3D::STRIPE_CONTRAST - 1.0e-6f && s <= 1.0f + Grass3D::STRIPE_CONTRAST + 1.0e-6f );
              }
          }
      }

    /*!
      the blades: in full near the camera, fading out, none far away
    */
    void blades()
      {
          QCOMPARE( Grass3D::bladesFade( 0.0f ), 1.0f );
          QCOMPARE( Grass3D::bladesFade( Grass3D::BLADES_NEAR ), 1.0f );
          QCOMPARE( Grass3D::bladesFade( ( Grass3D::BLADES_NEAR + Grass3D::BLADES_FAR ) / 2 ), 0.5f );
          QCOMPARE( Grass3D::bladesFade( Grass3D::BLADES_FAR ), 0.0f );
          QCOMPARE( Grass3D::bladesFade( 70.0f ), 0.0f ); // the broadcast camera: no reading
      }

    /*!
      painted into a plain ground image: toward the dry color in a goal
      mouth (redder, less green), the patches elsewhere (no redder, within
      PATCH_AMOUNT), the same image every time
    */
    void paint()
      {
          const double scale = 4.0; // pixels per meter
          const QColor field( 0x1f, 0xa0, 0x1f );
          QImage image( int( 115 * scale ), int( 78 * scale ), QImage::Format_RGBA8888 );
          image.fill( field );
          QImage painted = image;
          Grass3D::paintGround( painted, scale );
          QImage again = image;
          Grass3D::paintGround( again, scale );
          QCOMPARE( painted, again );
          const auto at = [&]( const QImage & im, const double x, const double z ) {
              return im.pixelColor( int( im.width() / 2 + x * scale ), int( im.height() / 2 + z * scale ) );
          };
          // the right goal mouth
          const QColor worn = at( painted, 49.5, 0.0 );
          QVERIFY2( worn.red() > field.red() + 15 && worn.green() < field.green(), qPrintable( worn.name() ) );
          // the patches elsewhere: lighter and darker, never redder than the patches allow
          int lighter = 0, darker = 0;
          for ( double x = -30.0; x <= 30.0; x += 1.5 )
          {
              for ( double z = -30.0; z <= -10.0; z += 1.5 )
              {
                  const QColor c = at( painted, x, z );
                  const double k = double( c.green() ) / field.green();
                  QVERIFY2( k > 1.0 - Grass3D::PATCH_AMOUNT / 2 - 0.02 && k < 1.0 + Grass3D::PATCH_AMOUNT / 2 + 0.02,
                            qPrintable( QString( "%1 %2 %3" ).arg( x ).arg( z ).arg( c.name() ) ) );
                  QVERIFY( std::abs( double( c.red() ) / field.red() - k ) < 0.08 ); // the same factor: not redder
                  if ( k > 1.02 ) ++lighter;
                  if ( k < 0.98 ) ++darker;
              }
          }
          QVERIFY( lighter > 10 && darker > 10 );
          // all the rows painted: near the bottom too
          int changed = 0;
          for ( double x = -50.0; x <= 50.0; x += 0.5 )
          {
              if ( at( painted, x, 37.5 ) != field ) ++changed;
          }
          QVERIFY2( changed > 150, qPrintable( QString::number( changed ) ) );
      }

    /*!
      the wear: most in the goal mouths, less at the penalty spots and the
      center, none elsewhere on the pitch; the same at both ends
    */
    void wear()
      {
          using Grass3D::wearAt;
          QCOMPARE( wearAt( -49.5f, 0.0f ), 1.0f );
          QCOMPARE( wearAt( 49.5f, 0.0f ), 1.0f );
          QVERIFY( std::fabs( wearAt( 41.5f, 0.0f ) - 0.6f ) < 0.15f );
          QVERIFY( std::fabs( wearAt( 0.0f, 0.0f ) - 0.5f ) < 1.0e-5f );
          QCOMPARE( wearAt( 25.0f, 20.0f ), 0.0f );
          QCOMPARE( wearAt( 0.0f, 30.0f ), 0.0f );
          QCOMPARE( wearAt( -52.0f, 30.0f ), 0.0f ); // the corner
          QVERIFY( wearAt( 49.5f, 6.0f ) > 0.0f && wearAt( 49.5f, 6.0f ) < 0.5f ); // fading out
          QVERIFY( wearAt( 39.5f, 0.0f ) > 0.0f ); // the edge of a penalty spot's
          QVERIFY( wearAt( 0.0f, 3.5f ) > 0.0f ); // and of the center's
          for ( float z = -10.0f; z <= 10.0f; z += 2.5f )
          {
              QCOMPARE( wearAt( -45.0f, z ), wearAt( 45.0f, z ) );
          }
      }
};

QTEST_MAIN( TestGrass3D )
#include "test_grass_3d.moc"
