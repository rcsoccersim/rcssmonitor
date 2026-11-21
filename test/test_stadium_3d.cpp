// -*-c++-*-

/*!
  \file test_stadium_3d.cpp
  \brief unit test of the surroundings of the pitch in the team colors
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

#include "options.h"
#include "stadium_3d.h"

#include <cmath>

namespace {

//! the pixels of an image near a color
int
countNear( const QImage & image,
           const QColor & color,
           const QRect & area )
{
    int n = 0;
    for ( int y = area.top(); y <= area.bottom(); ++y )
    {
        for ( int x = area.left(); x <= area.right(); ++x )
        {
            const QColor c = image.pixelColor( x, y );
            if ( std::abs( c.red() - color.red() ) + std::abs( c.green() - color.green() )
                 + std::abs( c.blue() - color.blue() ) < 30 )
            {
                ++n;
            }
        }
    }
    return n;
}

//! where the model matrix of a panel puts a point of the unit quad
QVector3D
at( const Instance3D & panel,
    const QVector3D & p = QVector3D() )
{
    return panel.model.map( p );
}

}

/*!
  \brief unit test of Stadium3D
*/
class TestStadium3D
    : public QObject {
    Q_OBJECT

private slots:

    void shadeAndKey()
      {
          QCOMPARE( Stadium3D::shade( QColor( 200, 100, 50 ), 0.5 ), QColor( 100, 50, 25 ) );
          QCOMPARE( Stadium3D::shade( QColor( 200, 100, 50 ), 2.0 ), QColor( 255, 200, 100 ) );

          Stadium3D::TeamLook a{ QColor( Qt::yellow ), "HELIOS", QImage() };
          Stadium3D::TeamLook b = a;
          QCOMPARE( a.key(), b.key() );
          b.color = Qt::cyan;
          QVERIFY( a.key() != b.key() );
          b = a;
          b.name = "CYRUS";
          QVERIFY( a.key() != b.key() );
          // the logo, and a change of its pixels
          b = a;
          b.logo = QImage( 16, 8, QImage::Format_ARGB32 );
          b.logo.fill( Qt::red );
          const QString with_logo = b.key();
          QVERIFY( a.key() != with_logo );
          b.logo.setPixelColor( 3, 3, Qt::blue );
          QVERIFY( b.key() != with_logo );
      }

    //! a tile of the boards: the dark shade, the edges and the name in the team color
    void board()
      {
          const QColor color( 255, 200, 0 );
          Stadium3D::TeamLook look{ color, "HELIOS2023", QImage() };
          const QImage image = Stadium3D::boardImage( look );
          QCOMPARE( image.size(), QSize( int( Stadium3D::BOARD_TILE * Stadium3D::BOARD_PIXELS_PER_METER ),
                                         int( Stadium3D::BOARD_HEIGHT * Stadium3D::BOARD_PIXELS_PER_METER ) ) );
          const QColor dark = Stadium3D::shade( color, Stadium3D::BOARD_SHADE );
          QCOMPARE( image.pixelColor( 3, 20 ), dark ); // left of the name
          QCOMPARE( image.pixelColor( 100, 1 ), color ); // the top edge
          QCOMPARE( image.pixelColor( 100, image.height() - 1 ), color ); // the bottom edge
          // the name in the middle, in the team color
          const QRect middle( image.width() / 4, 10, image.width() / 2, image.height() - 16 );
          QVERIFY( countNear( image, color, middle ) > 200 );
          // no name: no letters
          look.name.clear();
          QCOMPARE( countNear( Stadium3D::boardImage( look ), color, middle ), 0 );
      }

    //! the banner: a frame, the logo in whole pixels and the name beside it
    void banner()
      {
          const QColor color( 0, 200, 255 );
          QImage logo( 32, 16, QImage::Format_ARGB32 );
          logo.fill( Qt::white );
          Stadium3D::TeamLook look{ color, "CYRUS", logo };
          const QImage image = Stadium3D::bannerImage( look );
          QCOMPARE( image.size(), QSize( int( Stadium3D::BANNER_WIDTH * Stadium3D::BANNER_PIXELS_PER_METER ),
                                         int( Stadium3D::BANNER_HEIGHT * Stadium3D::BANNER_PIXELS_PER_METER ) ) );
          QCOMPARE( image.pixelColor( 2, image.height() / 2 ), color ); // the frame
          QCOMPARE( image.pixelColor( image.width() / 2, image.height() - 3 ), color );
          // the logo: white, as many whole times as fits 70% of the height
          const QRect all( 0, 0, image.width(), image.height() );
          const int k = int( std::floor( image.height() * 0.7 / logo.height() ) );
          QCOMPARE( countNear( image, Qt::white, all ), logo.width() * k * logo.height() * k );
          // the name to its right, in the team color
          int logo_right = 0;
          for ( int x = 0; x < image.width(); ++x ) if ( image.pixelColor( x, image.height() / 2 ) == QColor( Qt::white ) ) logo_right = x;
          QVERIFY( countNear( image, color, QRect( logo_right + 1, 20, image.width() - logo_right - 40, image.height() - 40 ) ) > 500 );
          QVERIFY( countNear( image, color, QRect( 20, 20, 60, image.height() - 40 ) ) == 0 ); // nothing at the far left

          // without the logo: the name in the middle
          look.logo = QImage();
          const QImage no_logo = Stadium3D::bannerImage( look );
          QCOMPARE( countNear( no_logo, Qt::white, all ), 0 );
          const int left = countNear( no_logo, color, QRect( 20, 20, image.width() / 2 - 20, image.height() - 40 ) );
          const int right = countNear( no_logo, color, QRect( image.width() / 2, 20, image.width() / 2 - 20, image.height() - 40 ) );
          QVERIFY( left > 100 && right > 100 );
      }

    /*!
      The panels: the boards along each half of the touch lines and behind
      each goal line, the banner high behind each goal, all facing the
      pitch, their backs in the dark shade of the end
    */
    void panels()
      {
          const QColor left( 255, 200, 0 ), right( 0, 200, 255 );
          SceneContent c;
          Stadium3D::addPanels( c, true, true, left, right );
          const float x = float( Options::PITCH_HALF_LENGTH ) + Stadium3D::BOARD_OFFSET;
          const float z = float( Options::PITCH_HALF_WIDTH ) + Stadium3D::BOARD_OFFSET;
          for ( int end = 0; end < 2; ++end )
          {
              const float side = ( end == 0 ? -1.0f : 1.0f );
              QCOMPARE( c.boards[end].size(), size_t( 3 ) );
              QCOMPARE( c.banners[end].size(), size_t( 1 ) );
              const QColor dark = Stadium3D::shade( end == 0 ? left : right, Stadium3D::BOARD_SHADE );
              for ( const auto * list : { &c.boards[end], &c.banners[end] } )
              {
                  for ( const Instance3D & p : *list )
                  {
                      QCOMPARE( p.color.toVector3D(), QVector3D( dark.redF(), dark.greenF(), dark.blueF() ) );
                      // the front (+z of the quad) toward the center of the pitch
                      const QVector3D center = at( p );
                      const QVector3D normal = at( p, QVector3D( 0, 0, 1 ) ) - center;
                      QVERIFY( QVector3D::dotProduct( normal, -QVector3D( center.x(), 0, center.z() ) ) > 0.0f );
                      // standing on the ground (the boards) or high (the banner)
                      QVERIFY( at( p, QVector3D( 0, -0.5f, 0 ) ).y() >= -1.0e-4f );
                  }
              }
              // along the touch lines: from the halfway line to the corner
              const Instance3D & near = c.boards[end][0];
              QVERIFY( std::fabs( at( near, QVector3D( -0.5f, 0, 0 ) ).x() * at( near, QVector3D( 0.5f, 0, 0 ) ).x() ) < 1.0e-3f );
              QVERIFY( std::fabs( std::fabs( at( near ).x() ) - x / 2.0f ) < 1.0e-4f );
              QVERIFY( at( near ).x() * side > 0.0f );
              QVERIFY( std::fabs( at( near ).z() - z ) < 1.0e-4f );
              QVERIFY( std::fabs( at( near, QVector3D( 0, 0.5f, 0 ) ).y() - Stadium3D::BOARD_HEIGHT ) < 1.0e-4f );
              // behind the goal line, across the pitch and its margins
              const Instance3D & goal_line = c.boards[end][2];
              QVERIFY( std::fabs( at( goal_line ).x() - side * x ) < 1.0e-4f );
              QVERIFY( std::fabs( ( at( goal_line, QVector3D( 0.5f, 0, 0 ) ) - at( goal_line, QVector3D( -0.5f, 0, 0 ) ) ).length() - 2.0f * z ) < 1.0e-3f );
              // the banner
              const Instance3D & banner = c.banners[end][0];
              QVERIFY( std::fabs( at( banner ).x() - side * ( float( Options::PITCH_HALF_LENGTH ) + Stadium3D::BANNER_BEHIND ) ) < 1.0e-4f );
              QVERIFY( std::fabs( at( banner, QVector3D( 0, -0.5f, 0 ) ).y() - Stadium3D::BANNER_BOTTOM ) < 1.0e-4f );
          }

          // each can be hidden
          c.clear();
          Stadium3D::addPanels( c, false, true, left, right );
          QVERIFY( c.boards[0].empty() && c.boards[1].empty() );
          QCOMPARE( c.banners[1].size(), size_t( 1 ) );
          c.clear();
          Stadium3D::addPanels( c, true, false, left, right );
          QVERIFY( c.banners[0].empty() );
          QCOMPARE( c.boards[0].size(), size_t( 3 ) );
      }

    //! the options: both on at first
    void options()
      {
          QVERIFY( Options::instance().boards3D() );
          QVERIFY( Options::instance().banners3D() );
      }
};

QTEST_MAIN( TestStadium3D )
#include "test_stadium_3d.moc"
