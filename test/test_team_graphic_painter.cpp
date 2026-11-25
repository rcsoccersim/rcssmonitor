// -*-c++-*-

/*!
  \file test_team_graphic_painter.cpp
  \brief unit test of TeamGraphicPainter: the team logos made apart from drawing them
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

#include "team_graphic_painter.h"

#include "disp_holder.h"
#include "options.h"

#include <QImage>
#include <QPainter>

#include <string>
#include <vector>

namespace {

//! a tile of the team graphic (8 x 8) in one color
std::vector< std::string >
tile( const char * color )
{
    std::vector< std::string > xpm;
    xpm.push_back( "8 8 1 1" );
    xpm.push_back( std::string( "a c " ) + color );
    for ( int i = 0; i < 8; ++i ) xpm.push_back( "aaaaaaaa" );
    return xpm;
}

//! a frame of the teams
void
addFrame( DispHolder & holder,
          const int time,
          const char * left,
          const char * right )
{
    rcss::rcg::TeamT team_l, team_r;
    team_l.name_ = left;
    team_r.name_ = right;
    holder.handleTeam( time, team_l, team_r );
    rcss::rcg::ShowInfoT show;
    show.time_ = time;
    holder.handleShow( show );
}

}

/*!
  \brief TeamGraphicPainter::update() makes the logos without drawing
  (docs/plan/2026-11-22-03_frame-export.md 4.2): the 3D view's ends use
  them before the HUD draws them, and the first frame of an export has them
*/
class TestTeamGraphicPainter
    : public QObject {
    Q_OBJECT

private slots:

    void initTestCase()
      {
          QVERIFY( Options::instance().showTeamGraphic() );
      }

    //! update() makes them from the tiles, without draw()
    void update()
      {
          DispHolder holder;
          addFrame( holder, 1, "A", "B" );
          holder.handleTeamGraphic( 'l', 0, 0, tile( "#ff0000" ) );
          holder.handleTeamGraphic( 'l', 1, 0, tile( "#ff0000" ) );
          holder.handleTeamGraphic( 'r', 0, 0, tile( "#0000ff" ) );
          holder.setIndexFirst();

          TeamGraphicPainter painter( holder );
          QVERIFY( painter.pixmap( true ).isNull() );
          QVERIFY( painter.pixmap( false ).isNull() );

          painter.update();
          QCOMPARE( painter.pixmap( true ).size(), QSize( 16, 8 ) );
          QCOMPARE( painter.pixmap( true ).toImage().pixelColor( 12, 4 ), QColor( "#ff0000" ) );
          QCOMPARE( painter.pixmap( false ).size(), QSize( 8, 8 ) );
          QCOMPARE( painter.pixmap( false ).toImage().pixelColor( 4, 4 ), QColor( "#0000ff" ) );

          // more tiles: made again
          holder.handleTeamGraphic( 'r', 0, 1, tile( "#00ff00" ) );
          painter.update();
          QCOMPARE( painter.pixmap( false ).size(), QSize( 8, 16 ) );
          QCOMPARE( painter.pixmap( false ).toImage().pixelColor( 4, 12 ), QColor( "#00ff00" ) );
      }

    //! draw() draws what update() made, at the top left (and right)
    void draw()
      {
          DispHolder holder;
          addFrame( holder, 1, "A", "B" );
          holder.handleTeamGraphic( 'l', 0, 0, tile( "#ff0000" ) );
          holder.setIndexFirst();

          TeamGraphicPainter painter( holder );
          QImage image( 200, 100, QImage::Format_RGB32 );
          image.fill( Qt::white );
          {
              QPainter p( &image );
              painter.draw( p );
          }
          QVERIFY( ! painter.pixmap( true ).isNull() );
          QCOMPARE( image.pixelColor( 2, 2 ), QColor( "#ff0000" ) );
      }

    //! nothing without the option, nor without a frame
    void optionOff()
      {
          DispHolder holder;
          addFrame( holder, 1, "A", "B" );
          holder.handleTeamGraphic( 'l', 0, 0, tile( "#ff0000" ) );
          holder.setIndexFirst();

          Options::instance().toggleShowTeamGraphic();
          TeamGraphicPainter painter( holder );
          painter.update();
          QVERIFY( painter.pixmap( true ).isNull() );
          Options::instance().toggleShowTeamGraphic();
          painter.update();
          QVERIFY( ! painter.pixmap( true ).isNull() );

          DispHolder empty;
          TeamGraphicPainter none( empty );
          none.update();
          QVERIFY( none.pixmap( true ).isNull() );
      }
};

QTEST_MAIN( TestTeamGraphicPainter )
#include "test_team_graphic_painter.moc"
