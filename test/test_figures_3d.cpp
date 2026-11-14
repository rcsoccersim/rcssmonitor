// -*-c++-*-

/*!
  \file test_figures_3d.cpp
  \brief unit test of Figures3D (the players as human figures)
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

#include "camera_mode.h"
#include "figures_3d.h"
#include "options.h"

#include <cmath>

namespace {

//! where a node of a figure is in the world
QVector3D
nodeAt( const Figures3D & figures,
        const FigureInstance & figure,
        const double time,
        const QString & name )
{
    std::vector< NodeTransform > local;
    figures.localPose( figure, time, local );
    std::vector< QMatrix4x4 > global;
    figures.model().globals( local, global );
    return ( figures.modelMatrix( figure ) * global[figures.model().nodeIndex( name )] ).column( 3 ).toVector3D();
}

//! a figure 2.22 m high (1.2 times a person's height) unless told
FigureInstance
figureAt( const int player,
          const float x,
          const float y,
          const float body,
          const float height = 2.22f )
{
    return FigureInstance{ player, x, y, body, height, QVector4D( 1.0f, 0.0f, 0.0f, 1.0f ) };
}

}

/*!
  \brief unit test of Figures3D
*/
class TestFigures3D
    : public QObject {
    Q_OBJECT

private:
    Figures3D M_figures;

private slots:

    void initTestCase()
      {
          QVERIFY2( M_figures.load(), qPrintable( M_figures.error() ) );
          QVERIFY( M_figures.ready() );
          QVERIFY( M_figures.load() ); // once
      }

    //! the option of the player model: simple by default, read by its names
    void option()
      {
          QCOMPARE( Options::PLAYER_MODEL_3D, PlayerModel::Simple );
          PlayerModel m = PlayerModel::Simple;
          QVERIFY( playerModelFromName( "figure", &m ) );
          QCOMPARE( m, PlayerModel::Figure );
          QVERIFY( ! playerModelFromName( "robot", &m ) );
          QCOMPARE( m, PlayerModel::Figure ); // unchanged
          QCOMPARE( playerModelName( PlayerModel::Simple ), QString( "simple" ) );
          QCOMPARE( Figures3D::jerseyMaterials(), QStringList( { "M_Main" } ) );

          // the scale of the figures: life size by default, kept within its range
          Options & opt = Options::instance();
          QCOMPARE( Options::FIGURE_SCALE_3D, 1.0 );
          opt.setFigureScale3D( 3.0 );
          QCOMPARE( opt.figureScale3D(), Options::MAX_FIGURE_SCALE_3D );
          opt.setFigureScale3D( 0.1 );
          QCOMPARE( opt.figureScale3D(), Options::MIN_FIGURE_SCALE_3D );
          opt.setFigureScale3D( Options::FIGURE_SCALE_3D );
      }

    /*!
      a figure stands on the ground at its player, 1.2 times a person's
      height, facing the body direction
    */
    void placed()
      {
          const FigureInstance f = figureAt( 3, 10.0f, -5.0f, 30.0f );
          // the lowest point of the model at rest on the ground at the player
          const QVector3D foot = M_figures.modelMatrix( f ).map( QVector3D( 0.0f, M_figures.model().min_y, 0.0f ) );
          QVERIFY( ( foot - QVector3D( 10.0f, 0.0f, -5.0f ) ).length() < 1.0e-4f );
          // as high as the figure's height, each its own
          for ( const float height : { 1.85f, 2.22f, 2.5f } )
          {
              const FigureInstance g = figureAt( 3, 10.0f, -5.0f, 30.0f, height );
              const QVector3D top = M_figures.modelMatrix( g ).map( QVector3D( 0.0f, M_figures.model().max_y, 0.0f ) );
              QVERIFY( std::fabs( top.y() - height ) < 1.0e-3f );
              const QVector3D low = M_figures.modelMatrix( g ).map( QVector3D( 0.0f, M_figures.model().min_y, 0.0f ) );
              QVERIFY( std::fabs( low.y() ) < 1.0e-4f );
          }

          // standing: the head high, the feet low
          const QVector3D head = nodeAt( M_figures, f, 0.0, "Head" );
          QVERIFY2( head.y() > 1.7f && head.y() < 2.2f, qPrintable( QString::number( head.y() ) ) );
          QVERIFY( nodeAt( M_figures, f, 0.0, "foot_l" ).y() < 0.3f );

          // facing the body direction (from +x toward +y on the field, which
          // is the world's +z): the toes ahead of the ankles, the left foot
          // to the left (toward -90 degrees from the body direction)
          for ( const float body : { 0.0f, 30.0f, 135.0f, -100.0f } )
          {
              const FigureInstance g = figureAt( 3, 10.0f, -5.0f, body );
              const float rad = body * float( M_PI ) / 180.0f;
              const QVector3D ahead( std::cos( rad ), 0.0f, std::sin( rad ) );
              const QVector3D left( std::sin( rad ), 0.0f, -std::cos( rad ) );
              const QVector3D toes = nodeAt( M_figures, g, 0.0, "ball_l" ) - nodeAt( M_figures, g, 0.0, "foot_l" );
              QVERIFY2( QVector3D::dotProduct( toes, ahead ) > 0.05f, qPrintable( QString::number( body ) ) );
              const QVector3D side = nodeAt( M_figures, g, 0.0, "thigh_l" ) - nodeAt( M_figures, g, 0.0, "thigh_r" );
              QVERIFY2( QVector3D::dotProduct( side, left ) > 0.1f, qPrintable( QString::number( body ) ) );
          }
      }

    /*!
      the skin matrices: as many as the joints of each figure, putting a
      joint's own point where its node is; the same for the same time, and
      the players at their own points of the idle clip
    */
    void pose()
      {
          SceneContent content;
          content.figures.push_back( figureAt( 0, 0.0f, 0.0f, 0.0f ) );
          content.figures.push_back( figureAt( 5, 20.0f, 3.0f, 90.0f ) );
          M_figures.pose( content, 12.3 );
          const std::size_t floats = std::size_t( M_figures.model().jointCount() ) * 16;
          QCOMPARE( content.figure_joints.size(), 2 * floats );

          // the joint of the head: its skin matrix takes the joint's point
          // at bind (the inverse of its inverse bind matrix) to the head
          const GltfModel & m = M_figures.model();
          const int head = m.nodeIndex( "Head" );
          const int joint = int( std::find( m.joint_nodes.begin(), m.joint_nodes.end(), head ) - m.joint_nodes.begin() );
          for ( int k = 0; k < 2; ++k )
          {
              const QMatrix4x4 skin = QMatrix4x4( &content.figure_joints[k * floats + joint * 16] ).transposed();
              const QVector3D bind = m.inverse_bind[joint].inverted().column( 3 ).toVector3D();
              QVERIFY( ( skin.map( bind ) - nodeAt( M_figures, content.figures[k], 12.3, "Head" ) ).length() < 1.0e-3f );
          }

          // the same time, the same pose (the log decides it, not the draws)
          const std::vector< float > first = content.figure_joints;
          M_figures.pose( content, 12.3 );
          QVERIFY( content.figure_joints == first );

          // the idle clip moves with the time, each player at its own point of it
          const FigureInstance a = figureAt( 0, 0.0f, 0.0f, 0.0f );
          const FigureInstance b = figureAt( 1, 0.0f, 0.0f, 0.0f );
          const QVector3D hand_a = nodeAt( M_figures, a, 1.0, "hand_l" );
          QVERIFY( ( hand_a - nodeAt( M_figures, a, 1.7, "hand_l" ) ).length() > 1.0e-4f );
          QVERIFY( ( hand_a - nodeAt( M_figures, b, 1.0, "hand_l" ) ).length() > 1.0e-4f );
          QVERIFY( ( hand_a - nodeAt( M_figures, b, 1.0 - Figures3D::IDLE_OFFSET, "hand_l" ) ).length() < 1.0e-4f );

          // no figures, no matrices
          content.figures.clear();
          M_figures.pose( content, 1.0 );
          QVERIFY( content.figure_joints.empty() );
      }
};

QTEST_MAIN( TestFigures3D )

#include "test_figures_3d.moc"
