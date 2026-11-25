// -*-c++-*-

/*!
  \file test_export_range_list.cpp
  \brief unit test of ExportRangeList: the ranges to export
*/

/*
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
*/

/////////////////////////////////////////////////////////////////////

#include <QtTest>

#include "export_range_list.h"

/*
  docs/plan/2026-11-22-03_frame-export.md 4.4.
*/

namespace {

QString
textOf( const ExportRangeList & list )
{
    QStringList texts;
    for ( int i = 0; i < list.size(); ++i )
    {
        const ExportRangeSpec & s = list.ranges()[i];
        texts << ( i == list.current() ? "*" : "" ) + s.name + "[" + s.start + "," + s.end + "]";
    }
    return texts.join( ' ' );
}

DispCont
framesOf( const int count )
{
    DispCont cont;
    for ( int c = 1; c <= count; ++c )
    {
        DispPtr disp( new rcss::rcg::DispInfoT() );
        disp->show_.time_ = c;
        cont.push_back( disp );
    }
    return cont;
}

}

/*!
  \brief unit test of ExportRangeList
*/
class TestExportRangeList
    : public QObject {
    Q_OBJECT

private slots:

    void edit();
    void addRemoveMove();
    void names();
    void resolve();
};

/*-------------------------------------------------------------------*/
//! one range of the whole log at first; the current one edited
void
TestExportRangeList::edit()
{
    ExportRangeList list;
    QSignalSpy changed( &list, &ExportRangeList::changed );
    QCOMPARE( textOf( list ), QString( "*range1[,]" ) );
    QVERIFY( list.isCleared() );

    list.setStart( "10" );
    list.setEnd( "20.1" );
    list.setName( "foul" );
    QCOMPARE( textOf( list ), QString( "*foul[10,20.1]" ) );
    QCOMPARE( changed.count(), 3 );
    list.setStart( "10" ); // the same: not told
    QCOMPARE( changed.count(), 3 );
    QVERIFY( ! list.isCleared() );

    list.setCurrentRange( ExportRangeSpec{ "1", "2", "a" } );
    QCOMPARE( textOf( list ), QString( "*a[1,2]" ) );

    list.clear();
    QCOMPARE( textOf( list ), QString( "*range1[,]" ) );
    QVERIFY( list.isCleared() );
    const int told = changed.count();
    list.clear(); // already: not told
    QCOMPARE( changed.count(), told );
}

/*-------------------------------------------------------------------*/
void
TestExportRangeList::addRemoveMove()
{
    ExportRangeList list;
    list.add( ExportRangeSpec{ "1", "2", "a" } );
    list.add( ExportRangeSpec{ "3", "4", "b" } );
    QCOMPARE( textOf( list ), QString( "range1[,] a[1,2] *b[3,4]" ) );

    list.setCurrent( 1 );
    QCOMPARE( textOf( list ), QString( "range1[,] *a[1,2] b[3,4]" ) );
    list.setCurrent( 5 ); // out: nothing
    QCOMPARE( list.current(), 1 );

    // moved: the current one stays current
    list.move( 1, 2 );
    QCOMPARE( textOf( list ), QString( "range1[,] b[3,4] *a[1,2]" ) );
    list.move( 0, 2 );
    QCOMPARE( textOf( list ), QString( "b[3,4] *a[1,2] range1[,]" ) );
    list.move( 2, 0 );
    QCOMPARE( textOf( list ), QString( "range1[,] b[3,4] *a[1,2]" ) );
    list.move( 0, 3 ); // out: nothing
    QCOMPARE( textOf( list ), QString( "range1[,] b[3,4] *a[1,2]" ) );

    // removed: the current one before it, or the last one
    list.remove( 2 );
    QCOMPARE( textOf( list ), QString( "range1[,] *b[3,4]" ) );
    list.remove( 0 );
    QCOMPARE( textOf( list ), QString( "*b[3,4]" ) );
    list.remove( 0 ); // the last one: the whole log
    QCOMPARE( textOf( list ), QString( "*range1[,]" ) );

    list.setRanges( { ExportRangeSpec{ "5", "6", "x" }, ExportRangeSpec{ "7", "8", "y" } } );
    QCOMPARE( textOf( list ), QString( "*x[5,6] y[7,8]" ) );
    list.setRanges( {} );
    QCOMPARE( textOf( list ), QString( "*range1[,]" ) );
}

/*-------------------------------------------------------------------*/
void
TestExportRangeList::names()
{
    ExportRangeList list;
    QCOMPARE( list.newName(), QString( "range2" ) );
    list.add( ExportRangeSpec{ "", "", "range2" } );
    QCOMPARE( list.newName(), QString( "range3" ) );
    QCOMPARE( list.newName( "goal" ), QString( "goal1" ) );

    QCOMPARE( ExportRangeList::safeName( "goal1-c1234" ), QString( "goal1-c1234" ) );
    QCOMPARE( ExportRangeList::safeName( " a/b c:d " ), QString( "a_b_c_d" ) );
    QCOMPARE( ExportRangeList::safeName( ".." ), QString( "range" ) );
    QCOMPARE( ExportRangeList::safeName( "" ), QString( "range" ) );
}

/*-------------------------------------------------------------------*/
//! the frames of a range: empty ends for the first and the last frames
void
TestExportRangeList::resolve()
{
    const DispCont frames = framesOf( 10 );
    QString error = "x";
    ExportRange r = ExportRangeList::resolve( ExportRangeSpec{ "", "", "" }, frames, &error );
    QCOMPARE( r.first, std::size_t( 0 ) );
    QCOMPARE( r.last, std::size_t( 9 ) );
    QVERIFY( error.isEmpty() );

    bool start_snapped = true;
    bool end_snapped = true;
    r = ExportRangeList::resolve( ExportRangeSpec{ "3", "5", "" }, frames, &error, &start_snapped, &end_snapped );
    QCOMPARE( r.first, std::size_t( 2 ) );
    QCOMPARE( r.last, std::size_t( 4 ) );
    QVERIFY( ! start_snapped );
    QVERIFY( ! end_snapped );

    r = ExportRangeList::resolve( ExportRangeSpec{ "0", "20", "" }, frames, &error, &start_snapped, &end_snapped );
    QVERIFY( r.isValid() );
    QVERIFY( start_snapped );
    QVERIFY( end_snapped );

    QVERIFY( ! ExportRangeList::resolve( ExportRangeSpec{ "5", "3", "" }, frames, &error ).isValid() );
    QCOMPARE( error, QString( "The start is after the end." ) );
    QVERIFY( ! ExportRangeList::resolve( ExportRangeSpec{ "11", "", "" }, frames, &error ).isValid() );
    QVERIFY2( error.contains( "after the last frame (10)" ), qPrintable( error ) );
    QVERIFY( ! ExportRangeList::resolve( ExportRangeSpec{ "", "0", "" }, frames, &error ).isValid() );
    QVERIFY2( error.contains( "before the first frame (1)" ), qPrintable( error ) );
    QVERIFY( ! ExportRangeList::resolve( ExportRangeSpec{ "x", "", "" }, frames, &error ).isValid() );
    QVERIFY2( error.contains( "Not a position" ), qPrintable( error ) );
    QVERIFY( ! ExportRangeList::resolve( ExportRangeSpec{ "", "", "" }, DispCont(), &error ).isValid() );
    QCOMPARE( error, QString( "There is no game log." ) );
}

QTEST_GUILESS_MAIN( TestExportRangeList )
#include "test_export_range_list.moc"
