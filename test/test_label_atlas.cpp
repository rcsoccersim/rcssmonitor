// -*-c++-*-

/*!
  \file test_label_atlas.cpp
  \brief unit test of LabelAtlas, the images of the number labels of the 3D view
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

#include "label_atlas.h"

#include <QFont>
#include <QImage>

namespace {

//! the part of the image of a slot
QImage
slotImage( const LabelAtlas & atlas,
           const int slot )
{
    const QRectF uv = atlas.uv( slot );
    const QImage & image = atlas.image();
    return image.copy( QRect( int( uv.left() * image.width() + 0.5 ), int( uv.top() * image.height() + 0.5 ),
                              int( uv.width() * image.width() + 0.5 ), int( uv.height() * image.height() + 0.5 ) ) );
}

//! how many pixels are white (the text) and how many are not transparent (the text and its outline)
void
countPixels( const QImage & image,
             int * white,
             int * opaque )
{
    *white = *opaque = 0;
    for ( int y = 0; y < image.height(); ++y )
    {
        for ( int x = 0; x < image.width(); ++x )
        {
            const QColor c = image.pixelColor( x, y );
            if ( c.alpha() > 200 ) ++*opaque;
            if ( c.alpha() > 200 && c.red() > 200 && c.green() > 200 && c.blue() > 200 ) ++*white;
        }
    }
}

}

class TestLabelAtlas
    : public QObject {
    Q_OBJECT

private:
    QFont M_font = QFont( "Sans Serif", 9, QFont::Bold );

private slots:

    void slotPlaces()
      {
          LabelAtlas atlas;
          // every slot is in the image, of the size of a label, and none overlaps another
          for ( int i = 0; i <= LabelAtlas::WHITE_SLOT; ++i )
          {
              const QRectF a = atlas.uv( i );
              QVERIFY( a.left() >= 0.0 && a.top() >= 0.0 && a.right() <= 1.0 && a.bottom() <= 1.0 );
              QCOMPARE( int( a.width() * atlas.image().width() + 0.5 ), LabelAtlas::LABEL_WIDTH );
              QCOMPARE( int( a.height() * atlas.image().height() + 0.5 ), LabelAtlas::LABEL_HEIGHT );
              for ( int j = 0; j < i; ++j )
              {
                  QVERIFY( ! a.intersects( atlas.uv( j ) ) );
              }
          }
      }

    void whiteSlot()
      {
          LabelAtlas atlas;
          int white = 0, opaque = 0;
          countPixels( slotImage( atlas, LabelAtlas::WHITE_SLOT ), &white, &opaque );
          QCOMPARE( white, LabelAtlas::LABEL_WIDTH * LabelAtlas::LABEL_HEIGHT );
          // the others are empty at first
          countPixels( slotImage( atlas, 0 ), &white, &opaque );
          QCOMPARE( opaque, 0 );
      }

    void text()
      {
          LabelAtlas atlas;
          atlas.takeChanged();
          QVERIFY( atlas.setText( 3, "10", M_font ) );
          QVERIFY( atlas.takeChanged() );

          // white text in a black outline, in its slot only
          int white = 0, opaque = 0;
          countPixels( slotImage( atlas, 3 ), &white, &opaque );
          QVERIFY2( white > 100, qPrintable( QString::number( white ) ) );
          QVERIFY( opaque > white + 100 );
          countPixels( slotImage( atlas, 2 ), &white, &opaque );
          QCOMPARE( opaque, 0 );
          countPixels( slotImage( atlas, 4 ), &white, &opaque );
          QCOMPARE( opaque, 0 );

          // centered: the middle column has the text, the edges do not
          const QImage image = slotImage( atlas, 3 );
          int middle = 0, edge = 0;
          for ( int y = 0; y < image.height(); ++y )
          {
              middle += ( image.pixelColor( image.width() / 2 - 8, y ).alpha() > 200 ? 1 : 0 )
                  + ( image.pixelColor( image.width() / 2 + 8, y ).alpha() > 200 ? 1 : 0 );
              edge += ( image.pixelColor( 2, y ).alpha() > 0 ? 1 : 0 )
                  + ( image.pixelColor( image.width() - 3, y ).alpha() > 0 ? 1 : 0 );
          }
          QVERIFY( middle > 10 );
          QCOMPARE( edge, 0 );

          // the same text again: not drawn again
          QVERIFY( ! atlas.setText( 3, "10", M_font ) );
          QVERIFY( ! atlas.takeChanged() );
          // another text or font: drawn again
          QVERIFY( atlas.setText( 3, "10,t2", M_font ) );
          QVERIFY( atlas.setText( 3, "10,t2", QFont( "Serif", 9 ) ) );
          QVERIFY( atlas.takeChanged() );

          // no slot out of range
          QVERIFY( ! atlas.setText( LabelAtlas::WHITE_SLOT, "1", M_font ) );
          QVERIFY( ! atlas.setText( -1, "1", M_font ) );
      }

    void width()
      {
          LabelAtlas atlas;
          atlas.setText( 0, "1", M_font );
          atlas.setText( 1, "11,t17", M_font );
          atlas.setText( 2, "10,t10,t10,t10,t10", M_font );
          // in meters at the label's size: wider for a longer text
          QVERIFY( atlas.textWidth( 0 ) > 0.1 );
          QVERIFY( atlas.textWidth( 1 ) > 2.0 * atlas.textWidth( 0 ) );
          // squeezed to the slot (4 pixels left at each side)
          const double most = ( LabelAtlas::LABEL_WIDTH - 8 ) * LabelAtlas::LABEL_SCALE / LabelAtlas::LABEL_HEIGHT;
          QVERIFY( std::fabs( atlas.textWidth( 2 ) - most ) < 1.0e-9 );
          int white = 0, opaque = 0;
          const QImage image = slotImage( atlas, 2 );
          for ( int y = 0; y < image.height(); ++y )
          {
              QCOMPARE( image.pixelColor( 0, y ).alpha(), 0 );
              QCOMPARE( image.pixelColor( image.width() - 1, y ).alpha(), 0 );
          }
          countPixels( image, &white, &opaque );
          QVERIFY( white > 100 );
          QCOMPARE( atlas.textWidth( 5 ), 0.0 );
      }
};

QTEST_MAIN( TestLabelAtlas )
#include "test_label_atlas.moc"
