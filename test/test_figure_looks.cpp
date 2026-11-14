// -*-c++-*-

/*!
  \file test_figure_looks.cpp
  \brief unit test of FigureLooks
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

#include <cmath>
#include <set>
#include <string>

using namespace FigureLooks;

namespace {

bool
same( const FigureLook & a,
      const FigureLook & b )
{
    return a.body == b.body && a.hair == b.hair && a.skin == b.skin && a.hair_color == b.hair_color;
}

bool
near( const QVector4D & a,
      const QVector4D & b )
{
    return ( a - b ).length() < 1.0e-5f;
}

//! one of a man's hairstyles, with a beard or not
bool
male( const FigureLook & l )
{
    const std::uint32_t style = l.hair & ~std::uint32_t( BEARD );
    return l.body == MALE && ( style == SIMPLE_PARTED || style == BUZZED );
}

//! one of a woman's hairstyles
bool
female( const FigureLook & l )
{
    return l.body == FEMALE && ( l.hair == BUNS || l.hair == LONG || l.hair == BUZZED_FEMALE );
}

}

/*!
  \brief unit test of FigureLooks: each player's looks as a human body
*/
class TestFigureLooks
    : public QObject {
    Q_OBJECT

private slots:

    //! the hairstyles by their materials' names
    void hairBits()
      {
          QCOMPARE( hairBit( "Hair_SimpleParted" ), std::uint32_t( SIMPLE_PARTED ) );
          QCOMPARE( hairBit( "Hair_Buzzed" ), std::uint32_t( BUZZED ) );
          QCOMPARE( hairBit( "Hair_Beard" ), std::uint32_t( BEARD ) );
          QCOMPARE( hairBit( "Hair_Buns" ), std::uint32_t( BUNS ) );
          QCOMPARE( hairBit( "Hair_Long" ), std::uint32_t( LONG ) );
          QCOMPARE( hairBit( "Hair_BuzzedFemale" ), std::uint32_t( BUZZED_FEMALE ) );
          QCOMPARE( hairBit( "Eyebrows" ), std::uint32_t( 0 ) );
          QCOMPARE( hairBit( "Hair_Mohawk" ), std::uint32_t( 0 ) );
      }

    //! the standard looks: a man's (a woman's with women only), the standard colors
    void standards()
      {
          const FigureLook plain;
          for ( const FigureBodies b : { FigureBodies::Mixed, FigureBodies::Male } )
          {
              const FigureLook l = standard( b );
              QCOMPARE( l.body, int( MALE ) );
              QCOMPARE( l.hair, std::uint32_t( SIMPLE_PARTED ) );
              QCOMPARE( l.skin, plain.skin );
              QCOMPARE( l.hair_color, plain.hair_color );
          }
          const FigureLook w = standard( FigureBodies::Female );
          QCOMPARE( w.body, int( FEMALE ) );
          QCOMPARE( w.hair, std::uint32_t( BUZZED_FEMALE ) );
          QCOMPARE( w.skin, plain.skin );
      }

    /*!
      a player's own looks: the same for the same player of the same team,
      others for the others; the bodies by the option, each with its own
      hairstyles; men and women mixed about half and half, a beard for
      about a third of the men; the skins and the hair colors of their lists
    */
    void own()
      {
          const FigureLook a = of( rcss::rcg::LEFT, 7, "HELIOS", FigureBodies::Mixed );
          QVERIFY( same( a, of( rcss::rcg::LEFT, 7, "HELIOS", FigureBodies::Mixed ) ) );
          QVERIFY( ! same( a, of( rcss::rcg::LEFT, 7, "CYRUS", FigureBodies::Mixed ) ) );
          QVERIFY( ! same( a, of( rcss::rcg::RIGHT, 7, "HELIOS", FigureBodies::Mixed ) ) );
          QVERIFY( ! same( a, of( rcss::rcg::LEFT, 8, "HELIOS", FigureBodies::Mixed ) ) );

          int women = 0, men = 0, beards = 0, n = 0;
          std::set< std::uint32_t > styles;
          std::set< float > hair_reds;
          float lightest = 0.0f, darkest = 1.0f;
          for ( const char * team : { "HELIOS", "CYRUS", "YuShan", "Oxsy", "", "FCPortugal", "WrightEagle", "MT", "Alice" } )
          {
              for ( const rcss::rcg::Side side : { rcss::rcg::LEFT, rcss::rcg::RIGHT } )
              {
                  for ( int unum = 1; unum <= 11; ++unum )
                  {
                      const FigureLook l = of( side, unum, team, FigureBodies::Mixed );
                      ++n;
                      QVERIFY( male( l ) || female( l ) );
                      if ( l.body == FEMALE ) ++women;
                      else
                      {
                          ++men;
                          if ( l.hair & BEARD ) ++beards;
                      }
                      styles.insert( l.hair & ~std::uint32_t( BEARD ) );
                      hair_reds.insert( l.hair_color.x() );
                      // the skin between the lightest and the darkest, red over green over blue
                      QVERIFY( l.skin.x() >= 0.40f - 1.0e-5f && l.skin.x() <= 0.96f + 1.0e-5f );
                      QVERIFY( l.skin.x() > l.skin.y() && l.skin.y() > l.skin.z() );
                      lightest = std::max( lightest, l.skin.x() );
                      darkest = std::min( darkest, l.skin.x() );

                      // one body only by the option
                      QVERIFY( male( of( side, unum, team, FigureBodies::Male ) ) );
                      QVERIFY( female( of( side, unum, team, FigureBodies::Female ) ) );
                  }
              }
          }
          QVERIFY2( women > n * 0.35 && women < n * 0.65, qPrintable( QString( "%1 of %2" ).arg( women ).arg( n ) ) );
          QVERIFY2( beards > men * 0.15 && beards < men * 0.45, qPrintable( QString( "%1 of %2" ).arg( beards ).arg( men ) ) );
          QCOMPARE( int( styles.size() ), 5 ); // all the hairstyles
          QCOMPARE( int( hair_reds.size() ), 6 ); // all the hair colors
          QVERIFY( lightest > 0.85f && darkest < 0.55f ); // light and dark skins
      }

    /*!
      the looks as far as known: the own ones for sure, the standard ones
      not at all, the colors between them, the body and the hairstyle the
      standard ones unless sure
    */
    void blended()
      {
          FigureLook own;
          own.body = FEMALE;
          own.hair = LONG;
          own.skin = QVector4D( 0.4f, 0.3f, 0.2f, 1.0f );
          own.hair_color = QVector4D( 0.8f, 0.6f, 0.4f, 1.0f );
          const FigureLook plain = standard( FigureBodies::Mixed );
          QVERIFY( same( blend( own, plain, 1.0f ), own ) );
          QVERIFY( same( blend( own, plain, 2.0f ), own ) );
          QVERIFY( same( blend( own, plain, 0.0f ), plain ) );
          QVERIFY( same( blend( own, plain, -1.0f ), plain ) );
          const FigureLook half = blend( own, plain, 0.5f );
          QCOMPARE( half.body, plain.body );
          QCOMPARE( half.hair, plain.hair );
          QVERIFY( near( half.skin, ( own.skin + plain.skin ) * 0.5f ) );
          QVERIFY( near( half.hair_color, ( own.hair_color + plain.hair_color ) * 0.5f ) );
          const FigureLook almost = blend( own, plain, 0.99f );
          QCOMPARE( almost.body, plain.body );
          QVERIFY( near( almost.skin, plain.skin + ( own.skin - plain.skin ) * 0.99f ) );
      }
};

QTEST_MAIN( TestFigureLooks )
#include "test_figure_looks.moc"
