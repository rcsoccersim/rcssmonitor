// -*-c++-*-

/*!
  \file test_tooltip_marker.cpp
  \brief unit test of TooltipMarker
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

#include "tooltip_marker.h"

#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

#include <functional>

/*!
  \brief test of the dotted underline under the texts with a tooltip
*/
class TestTooltipMarker
    : public QObject {
    Q_OBJECT

private:

    /*!
      \brief the pixels that differ between a widget drawn with and without
      a tooltip, as their bounding rectangle (empty if none)
    */
    QRect marked( QWidget * w )
      {
          const QString tip = w->toolTip();
          w->setToolTip( QString() );
          const QImage plain = w->grab().toImage();
          w->setToolTip( tip );
          const QImage with = w->grab().toImage();
          QRect diff;
          for ( int y = 0; y < plain.height(); ++y )
          {
              for ( int x = 0; x < plain.width(); ++x )
              {
                  if ( plain.pixel( x, y ) != with.pixel( x, y ) ) diff |= QRect( x, y, 1, 1 );
              }
          }
          return diff;
      }

private slots:

    /*!
      the labels, the check boxes, the radio buttons and the titles of the
      group boxes: a line just under their text, nowhere else; nothing
      without a tooltip, nothing on a push button; a tooltip set later shows
    */
    void widgets()
      {
          QWidget root;
          QVBoxLayout * layout = new QVBoxLayout( &root );
          QLabel * label = new QLabel( "A label:" );
          QCheckBox * check = new QCheckBox( "A check box" );
          QRadioButton * radio = new QRadioButton( "A radio button" );
          QGroupBox * group = new QGroupBox( "A group" );
          QVBoxLayout * inner = new QVBoxLayout( group );
          inner->addWidget( new QLabel( "inside" ) );
          QPushButton * push = new QPushButton( "A button" );
          QLabel * later = new QLabel( "Later:" );
          for ( QWidget * w : std::initializer_list< QWidget * >{ label, check, radio, group, push, later } )
          {
              layout->addWidget( w );
          }
          for ( QWidget * w : std::initializer_list< QWidget * >{ label, check, radio, group, push } )
          {
              w->setToolTip( "details" );
          }
          TooltipMarker::install( &root );
          root.show();
          QVERIFY( QTest::qWaitForWindowExposed( &root ) || true );

          for ( QWidget * w : std::initializer_list< QWidget * >{ label, check, radio, group } )
          {
              const QRect text = TooltipMarker::textRect( w );
              QVERIFY2( ! text.isEmpty(), w->metaObject()->className() );
              const QRect diff = marked( w );
              QVERIFY2( ! diff.isEmpty(), w->metaObject()->className() );
              // under the text: within its width, below the middle of its line
              QVERIFY2( diff.left() >= text.left() - 1 && diff.right() <= text.right() + 1,
                        qPrintable( QString( "%1: %2-%3 %4-%5" ).arg( w->metaObject()->className() )
                                    .arg( diff.left() ).arg( diff.right() ).arg( text.left() ).arg( text.right() ) ) );
              QVERIFY2( diff.top() > text.center().y() && diff.bottom() <= text.bottom() + 4,
                        qPrintable( QString( "%1: %2-%3 %4" ).arg( w->metaObject()->className() )
                                    .arg( diff.top() ).arg( diff.bottom() ).arg( text.bottom() ) ) );
              QVERIFY( diff.height() <= 2 ); // a line
              QVERIFY( diff.width() > text.width() / 2 ); // along the text
          }
          // no line on a push button, nor without a tooltip
          QVERIFY( TooltipMarker::textRect( push ).isEmpty() );
          QVERIFY( marked( push ).isEmpty() );
          QVERIFY( marked( later ).isEmpty() );
          // a tooltip set after the marker was installed
          later->setToolTip( "details" );
          QVERIFY( ! marked( later ).isEmpty() );
      }

    /*!
      a disabled item is marked too (fainter), and a label of rich text or
      wrapped text is not
    */
    void others()
      {
          QWidget root;
          QVBoxLayout * layout = new QVBoxLayout( &root );
          QCheckBox * off = new QCheckBox( "Disabled" );
          off->setEnabled( false );
          QLabel * rich = new QLabel( "<b>rich</b> text" );
          QLabel * wrapped = new QLabel( "a long text that wraps" );
          wrapped->setWordWrap( true );
          for ( QWidget * w : std::initializer_list< QWidget * >{ off, rich, wrapped } )
          {
              layout->addWidget( w );
              w->setToolTip( "details" );
          }
          TooltipMarker::install( &root );
          root.show();
          QVERIFY( ! marked( off ).isEmpty() );
          QVERIFY( marked( rich ).isEmpty() );
          QVERIFY( marked( wrapped ).isEmpty() );
      }
};

QTEST_MAIN( TestTooltipMarker )
#include "test_tooltip_marker.moc"
