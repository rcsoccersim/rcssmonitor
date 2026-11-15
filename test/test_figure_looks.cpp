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

//! the bodies of a cast as the Human pack's: a man and a woman
constexpr int MAN = 0;
constexpr int WOMAN = 1;
//! the man's hairstyles and his beard (bits of his body)
constexpr std::uint32_t SIMPLE_PARTED = 1u << 0;
constexpr std::uint32_t BUZZED = 1u << 1;
constexpr std::uint32_t BEARD = 1u << 2;
//! the woman's hairstyles (bits of her body)
constexpr std::uint32_t BUNS = 1u << 0;
constexpr std::uint32_t LONG = 1u << 1;
constexpr std::uint32_t BUZZED_FEMALE = 1u << 2;

//! as FigurePack::cast() of the Human pack (test_figure_pack checks that it is)
FigureCast
humanCast()
{
    FigureCast cast;
    FigureCast::Body man;
    man.sex = FigureSex::Male;
    man.hairstyles = { SIMPLE_PARTED, BUZZED };
    man.standard_hair = SIMPLE_PARTED;
    man.extras = { FigureCast::Extra{ BEARD, 0.3 } }; // as the Human pack's figure.json
    FigureCast::Body woman;
    woman.sex = FigureSex::Female;
    woman.hairstyles = { BUNS, LONG, BUZZED_FEMALE };
    woman.standard_hair = BUZZED_FEMALE;
    cast.bodies = { man, woman };
    return cast;
}

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
    const std::uint32_t style = l.hair & ~BEARD;
    return l.body == MAN && ( style == SIMPLE_PARTED || style == BUZZED );
}

//! one of a woman's hairstyles
bool
female( const FigureLook & l )
{
    return l.body == WOMAN && ( l.hair == BUNS || l.hair == LONG || l.hair == BUZZED_FEMALE );
}

//! the looks of 198 players: 9 teams, both sides, 11 each
template < typename F >
void
forPlayers( F f )
{
    for ( const char * team : { "HELIOS", "CYRUS", "YuShan", "Oxsy", "", "FCPortugal", "WrightEagle", "MT", "Alice" } )
    {
        for ( const rcss::rcg::Side side : { rcss::rcg::LEFT, rcss::rcg::RIGHT } )
        {
            for ( int unum = 1; unum <= 11; ++unum )
            {
                f( side, unum, team );
            }
        }
    }
}

}

/*!
  \brief unit test of FigureLooks: each player's looks as a human body
*/
class TestFigureLooks
    : public QObject {
    Q_OBJECT

private slots:

    /*!
      the standard looks: the first man's body (the first woman's with
      women only) with its standard hairstyle, the standard colors
    */
    void standards()
      {
          const FigureCast cast = humanCast();
          const FigureLook plain;
          for ( const FigureBodies b : { FigureBodies::Mixed, FigureBodies::Male } )
          {
              const FigureLook l = standard( b, cast );
              QCOMPARE( l.body, MAN );
              QCOMPARE( l.hair, SIMPLE_PARTED );
              QCOMPARE( l.skin, plain.skin );
              QCOMPARE( l.hair_color, plain.hair_color );
          }
          const FigureLook w = standard( FigureBodies::Female, cast );
          QCOMPARE( w.body, WOMAN );
          QCOMPARE( w.hair, BUZZED_FEMALE );
          QCOMPARE( w.skin, plain.skin );

          // a body of anyone's without hairstyles (the Mannequin); the
          // woman first; no woman's body: the first; no bodies at all
          FigureCast one;
          one.bodies.resize( 1 );
          QCOMPARE( standard( FigureBodies::Female, one ).body, 0 );
          QCOMPARE( standard( FigureBodies::Female, one ).hair, std::uint32_t( 0 ) );
          FigureCast swapped = cast;
          std::swap( swapped.bodies[0], swapped.bodies[1] );
          QCOMPARE( standard( FigureBodies::Mixed, swapped ).body, 1 );
          QCOMPARE( standard( FigureBodies::Female, swapped ).body, 0 );
          FigureCast men = cast;
          men.bodies.pop_back();
          QCOMPARE( standard( FigureBodies::Female, men ).body, 0 );
          QCOMPARE( standard( FigureBodies::Mixed, FigureCast() ).body, 0 );
          QCOMPARE( standard( FigureBodies::Mixed, FigureCast() ).hair, std::uint32_t( 0 ) );
      }

    /*!
      a player's own looks: the same for the same player of the same team,
      others for the others; the bodies by the option, each with its own
      hairstyles; men and women mixed about half and half, a beard for
      about a third of the men; the skins and the hair colors of their lists
    */
    void own()
      {
          const FigureCast cast = humanCast();
          const FigureLook a = of( rcss::rcg::LEFT, 7, "HELIOS", FigureBodies::Mixed, cast );
          QVERIFY( same( a, of( rcss::rcg::LEFT, 7, "HELIOS", FigureBodies::Mixed, cast ) ) );
          QVERIFY( ! same( a, of( rcss::rcg::LEFT, 7, "CYRUS", FigureBodies::Mixed, cast ) ) );
          QVERIFY( ! same( a, of( rcss::rcg::RIGHT, 7, "HELIOS", FigureBodies::Mixed, cast ) ) );
          QVERIFY( ! same( a, of( rcss::rcg::LEFT, 8, "HELIOS", FigureBodies::Mixed, cast ) ) );

          int women = 0, men = 0, beards = 0, n = 0;
          std::set< std::uint32_t > styles;
          std::set< float > hair_reds;
          float lightest = 0.0f, darkest = 1.0f;
          forPlayers( [&]( const rcss::rcg::Side side, const int unum, const char * team ) {
                  const FigureLook l = of( side, unum, team, FigureBodies::Mixed, cast );
                  ++n;
                  QVERIFY( male( l ) || female( l ) );
                  if ( l.body == WOMAN ) ++women;
                  else
                  {
                      ++men;
                      if ( l.hair & BEARD ) ++beards;
                  }
                  styles.insert( ( l.body << 8 ) | ( l.hair & ~( l.body == MAN ? BEARD : 0u ) ) );
                  hair_reds.insert( l.hair_color.x() );
                  // the skin between the lightest and the darkest, red over green over blue
                  QVERIFY( l.skin.x() >= 0.40f - 1.0e-5f && l.skin.x() <= 0.96f + 1.0e-5f );
                  QVERIFY( l.skin.x() > l.skin.y() && l.skin.y() > l.skin.z() );
                  lightest = std::max( lightest, l.skin.x() );
                  darkest = std::min( darkest, l.skin.x() );

                  // one body only by the option
                  QVERIFY( male( of( side, unum, team, FigureBodies::Male, cast ) ) );
                  QVERIFY( female( of( side, unum, team, FigureBodies::Female, cast ) ) );
              } );
          QVERIFY2( women > n * 0.35 && women < n * 0.65, qPrintable( QString( "%1 of %2" ).arg( women ).arg( n ) ) );
          QVERIFY2( beards > men * 0.15 && beards < men * 0.45, qPrintable( QString( "%1 of %2" ).arg( beards ).arg( men ) ) );
          QCOMPARE( int( styles.size() ), 5 ); // all the hairstyles
          QCOMPARE( int( hair_reds.size() ), 6 ); // all the hair colors
          QVERIFY( lightest > 0.85f && darkest < 0.55f ); // light and dark skins
      }

    /*!
      other packs' bodies: one of anyone's without hairstyles (the
      Mannequin); bodies of a sex picked as often each; no body of the
      option's sex (all of them); bodies of anyone's in both; extras each
      by its chance, apart; the colors and the first body as for the Human
      (as many numbers drawn for any pack)
    */
    void otherCasts()
      {
          const FigureCast human = humanCast();

          FigureCast one;
          one.bodies.resize( 1 );
          forPlayers( [&]( const rcss::rcg::Side side, const int unum, const char * team ) {
                  for ( const FigureBodies b : { FigureBodies::Mixed, FigureBodies::Male, FigureBodies::Female } )
                  {
                      const FigureLook l = of( side, unum, team, b, one );
                      QCOMPARE( l.body, 0 );
                      QCOMPARE( l.hair, std::uint32_t( 0 ) );
                      // the colors as the Human's
                      const FigureLook h = of( side, unum, team, b, human );
                      QCOMPARE( l.skin, h.skin );
                      QCOMPARE( l.hair_color, h.hair_color );
                  }
              } );
          QCOMPARE( of( rcss::rcg::LEFT, 1, "A", FigureBodies::Mixed, FigureCast() ).body, 0 );
          QCOMPARE( of( rcss::rcg::LEFT, 1, "A", FigureBodies::Mixed, FigureCast() ).hair, std::uint32_t( 0 ) );

          // two men and a woman (after them): each man about as often
          FigureCast three = human;
          std::swap( three.bodies[0], three.bodies[1] );
          three.bodies.insert( three.bodies.begin(), three.bodies[1] ); // man, woman, man
          three.bodies[0].hairstyles = { 1u << 0 }; // to tell the two men apart
          three.bodies[0].standard_hair = 1u << 0;
          int counts[3] = { 0, 0, 0 };
          int women_only = 0, men_only[3] = { 0, 0, 0 };
          forPlayers( [&]( const rcss::rcg::Side side, const int unum, const char * team ) {
                  const FigureLook l = of( side, unum, team, FigureBodies::Mixed, three );
                  QVERIFY( l.body >= 0 && l.body < 3 );
                  ++counts[l.body];
                  // the same lot: a woman where the Human's is a woman
                  QCOMPARE( l.body == 1, of( side, unum, team, FigureBodies::Mixed, human ).body == WOMAN );
                  if ( of( side, unum, team, FigureBodies::Female, three ).body == 1 ) ++women_only;
                  ++men_only[of( side, unum, team, FigureBodies::Male, three ).body];
              } );
          QVERIFY2( counts[0] > 30 && counts[2] > 30 && counts[1] > 60,
                    qPrintable( QString( "%1 %2 %3" ).arg( counts[0] ).arg( counts[1] ).arg( counts[2] ) ) );
          QCOMPARE( women_only, 198 );
          QCOMPARE( men_only[1], 0 );
          QVERIFY( men_only[0] > 70 && men_only[2] > 70 );

          // men only in the pack: the option of women takes them all
          FigureCast men = human;
          men.bodies.pop_back();
          forPlayers( [&]( const rcss::rcg::Side side, const int unum, const char * team ) {
                  QCOMPARE( of( side, unum, team, FigureBodies::Female, men ).body, 0 );
                  QCOMPARE( of( side, unum, team, FigureBodies::Mixed, men ).body, 0 );
              } );

          // a body of anyone's goes with men and with women
          FigureCast any = human;
          any.bodies.push_back( FigureCast::Body() );
          bool any_man = false, any_woman = false;
          forPlayers( [&]( const rcss::rcg::Side side, const int unum, const char * team ) {
                  any_man = any_man || of( side, unum, team, FigureBodies::Male, any ).body == 2;
                  any_woman = any_woman || of( side, unum, team, FigureBodies::Female, any ).body == 2;
                  QVERIFY( of( side, unum, team, FigureBodies::Male, any ).body != WOMAN );
                  QVERIFY( of( side, unum, team, FigureBodies::Female, any ).body != MAN );
              } );
          QVERIFY( any_man && any_woman );

          // two extras: each by its own chance, apart from each other
          FigureCast extras = human;
          extras.bodies[MAN].extras = { FigureCast::Extra{ 1u << 2, 0.5 }, FigureCast::Extra{ 1u << 3, 0.5 } };
          FigureCast half_beard = human;
          half_beard.bodies[MAN].extras[0].chance = 0.5;
          int first = 0, second = 0, both = 0, men_n = 0;
          forPlayers( [&]( const rcss::rcg::Side side, const int unum, const char * team ) {
                  const FigureLook l = of( side, unum, team, FigureBodies::Male, extras );
                  ++men_n;
                  if ( l.hair & ( 1u << 2 ) ) ++first;
                  if ( l.hair & ( 1u << 3 ) ) ++second;
                  if ( ( l.hair & ( 1u << 2 ) ) && ( l.hair & ( 1u << 3 ) ) ) ++both;
                  // the first extra by the same number as the Human's beard
                  QCOMPARE( bool( l.hair & ( 1u << 2 ) ),
                            bool( of( side, unum, team, FigureBodies::Male, half_beard ).hair & BEARD ) );
              } );
          QVERIFY2( first > men_n * 0.35 && first < men_n * 0.65, qPrintable( QString::number( first ) ) );
          QVERIFY2( second > men_n * 0.35 && second < men_n * 0.65, qPrintable( QString::number( second ) ) );
          QVERIFY2( both > men_n * 0.12 && both < men_n * 0.38, qPrintable( QString::number( both ) ) );
      }

    /*!
      the looks as far as known: the own ones for sure, the standard ones
      not at all, the colors between them, the body and the hairstyle the
      standard ones unless sure
    */
    void blended()
      {
          FigureLook own;
          own.body = WOMAN;
          own.hair = LONG;
          own.skin = QVector4D( 0.4f, 0.3f, 0.2f, 1.0f );
          own.hair_color = QVector4D( 0.8f, 0.6f, 0.4f, 1.0f );
          const FigureLook plain = standard( FigureBodies::Mixed, humanCast() );
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
