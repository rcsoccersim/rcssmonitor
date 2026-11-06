// -*-c++-*-

/*!
  \file test_message_splitter.cpp
  \brief unit test of MessageSplitter
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

#include "message_splitter.h"

namespace {

QByteArray
bytes( const char * data,
       const int size )
{
    return QByteArray( data, size );
}

}

class TestMessageSplitter
    : public QObject {

    Q_OBJECT

private slots:

    void splitAcrossReads();
    void multipleMessagesInOneRead();
    void discardEmptyMessages();
    void tooLongPartialMessage();
    void popFromEmpty();
    void nulTerminated();
    void clear();
};

/*-------------------------------------------------------------------*/
void
TestMessageSplitter::splitAcrossReads()
{
    MessageSplitter splitter( 1024 );
    QByteArray msg;

    QVERIFY( splitter.append( "(show 1" ) );
    QVERIFY( ! splitter.hasMessage() );
    QCOMPARE( splitter.partialSize(), 7 );

    QVERIFY( splitter.append( " (b 0 0)" ) );
    QVERIFY( ! splitter.hasMessage() );

    QVERIFY( splitter.append( bytes( ")\0", 2 ) ) );
    QVERIFY( splitter.hasMessage() );
    QCOMPARE( splitter.partialSize(), 0 );

    QVERIFY( splitter.pop( msg ) );
    QCOMPARE( msg, QByteArray( "(show 1 (b 0 0))" ) );
    QVERIFY( ! splitter.pop( msg ) );
}

/*-------------------------------------------------------------------*/
void
TestMessageSplitter::multipleMessagesInOneRead()
{
    MessageSplitter splitter( 1024 );
    QByteArray msg;

    QVERIFY( splitter.append( bytes( "(a)\0(b)\0(c", 10 ) ) );
    QCOMPARE( splitter.partialSize(), 2 );

    QVERIFY( splitter.pop( msg ) );
    QCOMPARE( msg, QByteArray( "(a)" ) );
    QVERIFY( splitter.pop( msg ) );
    QCOMPARE( msg, QByteArray( "(b)" ) );
    QVERIFY( ! splitter.pop( msg ) );

    QVERIFY( splitter.append( bytes( ")\0", 2 ) ) );
    QVERIFY( splitter.pop( msg ) );
    QCOMPARE( msg, QByteArray( "(c)" ) );
    QVERIFY( ! splitter.pop( msg ) );
}

/*-------------------------------------------------------------------*/
void
TestMessageSplitter::discardEmptyMessages()
{
    MessageSplitter splitter( 1024 );
    QByteArray msg;

    QVERIFY( splitter.append( bytes( "\0\0(a)\0\0\0(b)\0\0", 13 ) ) );

    QVERIFY( splitter.pop( msg ) );
    QCOMPARE( msg, QByteArray( "(a)" ) );
    QVERIFY( splitter.pop( msg ) );
    QCOMPARE( msg, QByteArray( "(b)" ) );
    QVERIFY( ! splitter.pop( msg ) );
    QCOMPARE( splitter.partialSize(), 0 );
}

/*-------------------------------------------------------------------*/
void
TestMessageSplitter::tooLongPartialMessage()
{
    MessageSplitter splitter( 8 );
    QByteArray msg;

    // a complete message longer than the limit is accepted.
    QVERIFY( splitter.append( bytes( "0123456789\0", 11 ) ) );
    QVERIFY( splitter.pop( msg ) );
    QCOMPARE( msg, QByteArray( "0123456789" ) );

    // an incomplete message up to the limit is accepted.
    QVERIFY( splitter.append( "0123" ) );
    QVERIFY( splitter.append( "4567" ) );
    QCOMPARE( splitter.partialSize(), 8 );

    // an incomplete message over the limit is an error.
    QVERIFY( ! splitter.append( "8" ) );
}

/*-------------------------------------------------------------------*/
void
TestMessageSplitter::popFromEmpty()
{
    MessageSplitter splitter( 1024 );
    QByteArray msg( "garbage" );

    QVERIFY( ! splitter.hasMessage() );
    QVERIFY( ! splitter.pop( msg ) );
    QVERIFY( msg.isEmpty() );

    QVERIFY( splitter.append( QByteArray() ) );
    QVERIFY( ! splitter.hasMessage() );
}

/*-------------------------------------------------------------------*/
void
TestMessageSplitter::nulTerminated()
{
    MessageSplitter splitter( 1024 );
    QByteArray msg;

    QVERIFY( splitter.append( bytes( "(a b c)\0", 8 ) ) );
    QVERIFY( splitter.pop( msg ) );
    QCOMPARE( msg.size(), 7 );
    QCOMPARE( msg.constData()[msg.size()], '\0' );
    QCOMPARE( qstrcmp( msg.constData(), "(a b c)" ), 0 );
}

/*-------------------------------------------------------------------*/
void
TestMessageSplitter::clear()
{
    MessageSplitter splitter( 1024 );
    QByteArray msg;

    QVERIFY( splitter.append( bytes( "(a)\0(b", 6 ) ) );
    splitter.clear();
    QVERIFY( ! splitter.hasMessage() );
    QCOMPARE( splitter.partialSize(), 0 );

    QVERIFY( splitter.append( bytes( ")\0", 2 ) ) );
    QVERIFY( splitter.pop( msg ) );
    QCOMPARE( msg, QByteArray( ")" ) );
}

QTEST_APPLESS_MAIN( TestMessageSplitter )

#include "test_message_splitter.moc"
