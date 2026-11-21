// -*-c++-*-

/*!
  \file test_fps_meter.cpp
  \brief unit test of the frames per second shown over the 3D view
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

#include "fps_meter.h"

#include <cmath>

/*!
  \brief unit test of FpsMeter
*/
class TestFpsMeter
    : public QObject {
    Q_OBJECT

private slots:

    /*!
      the frames over the last second: 60 a second, then a pause; still
      ("-") with fewer than two frames in the window, or when told so
    */
    void fps()
      {
          FpsMeter m;
          QVERIFY( m.fps( 0.0 ) < 0.0 );
          QCOMPARE( m.text( 0.0 ), QString( "- fps" ) );
          m.frame( 0.0 );
          QVERIFY( m.fps( 0.0 ) < 0.0 ); // one frame
          for ( int k = 1; k <= 120; ++k ) m.frame( k / 60.0 );
          QVERIFY( std::fabs( m.fps( 2.0 ) - 60.0 ) < 1.0e-6 );
          QCOMPARE( m.text( 2.0 ), QString( "60 fps" ) );
          QCOMPARE( m.text( 2.0, true ), QString( "- fps" ) ); // the last frame before stopping
          // half a second later, half of the window: still 60 a second
          QVERIFY( std::fabs( m.fps( 2.5 ) - 60.0 ) < 1.0e-6 );
          // a pause: no frames in the last second
          QVERIFY( m.fps( 3.5 ) < 0.0 );
          // then 30 a second; the old ones leave the window
          for ( int k = 0; k <= 30; ++k ) m.frame( 10.0 + k / 30.0 );
          QVERIFY( std::fabs( m.fps( 11.0 ) - 30.0 ) < 1.0e-6 );
          m.clear();
          QVERIFY( m.fps( 11.0 ) < 0.0 );
      }

    /*!
      the GPU's time of a frame: of the last times that came, none before
      any; in the text with one decimal
    */
    void gpu()
      {
          FpsMeter m;
          QVERIFY( m.gpuTime() < 0.0 );
          m.gpu( 0.0, 0 ); // none came
          QVERIFY( m.gpuTime() < 0.0 );
          m.gpu( 22.4, 4 );
          QCOMPARE( m.gpuTime(), 5.6 );
          m.gpu( 0.0, 0 ); // none this frame: the last kept
          QCOMPARE( m.gpuTime(), 5.6 );
          for ( int k = 0; k <= 60; ++k ) m.frame( k / 60.0 );
          QCOMPARE( m.text( 1.0 ), QString::fromUtf8( "60 fps · GPU 5.6 ms" ) );
          QCOMPARE( m.text( 1.0, true ), QString::fromUtf8( "- fps · GPU 5.6 ms" ) );
          m.clear();
          QVERIFY( m.gpuTime() < 0.0 );
      }
};

QTEST_MAIN( TestFpsMeter )

#include "test_fps_meter.moc"
