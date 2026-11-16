// -*-c++-*-

/*!
  \file test_figure_pack.cpp
  \brief unit test of the figure packs (FigurePack, FigurePacks, Figures3D::setPack()).
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

#include "figure_pack.h"
#include "figures_3d.h"
#include "test_bvh.h"
#include "test_gltf.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTemporaryDir>

#include <algorithm>
#include <cmath>

namespace {

//! write a file
bool
writeFile( const QString & path,
           const QByteArray & data )
{
    QDir().mkpath( QFileInfo( path ).absolutePath() );
    QFile f( path );
    return f.open( QIODevice::WriteOnly ) && f.write( data ) == data.size();
}

//! a pack's folder: figure.json, and the Mannequin's model under these names
QString
makePack( const QString & folder,
          const QString & name,
          const QByteArray & json,
          const QStringList & models = QStringList() )
{
    const QString dir = QDir( folder ).filePath( name );
    QDir().mkpath( dir );
    for ( const QString & m : models )
    {
        QFile::remove( QDir( dir ).filePath( m ) );
        QFile::copy( ":/figures/mannequin/Mannequin.glb", QDir( dir ).filePath( m ) );
    }
    if ( ! json.isEmpty() ) writeFile( QDir( dir ).filePath( "figure.json" ), json );
    return dir;
}

//! a figure.json of the format 1: the Mannequin's model (Body.glb) and the bundled motion, with more in the body
QByteArray
packJson( const QByteArray & body_extra = QByteArray(),
          const QByteArray & motion = "\"mannequin\"",
          const QByteArray & top_extra = QByteArray() )
{
    return "{ \"format\": 1, \"name\": \"A Pack\", " + top_extra + "\n"
        "  \"motion\": " + motion + ",\n"
        "  \"bodies\": [ { \"model\": \"Body.glb\", \"paint\": { \"M_Main\": \"team\" }" + body_extra + " } ] }\n";
}

//! the motion of the Mannequin's clips in the pack's own file
const QByteArray OWN_MOTION = "{ \"model\": \"Motion.glb\", \"skeleton\": \"quaternius\", \"clips\": "
    "{ \"idle\": \"Idle_Loop\", \"walk\": \"Walk_Loop\", \"jog\": \"Jog_Fwd_Loop\", \"sprint\": \"Sprint_Loop\" } }";

const FigurePack *
byId( const std::vector< FigurePack > & list,
      const QString & id )
{
    for ( const FigurePack & p : list )
    {
        if ( p.id == id ) return &p;
    }
    return nullptr;
}

}

/*!
  \brief unit test of the figure packs
*/
class TestFigurePack
    : public QObject {
    Q_OBJECT

private:
    QTemporaryDir M_data; //!< the XDG data folders

private slots:

    void initTestCase()
      {
          QVERIFY( M_data.isValid() );
          // the user's data folder of packs here, none of the system's
          qputenv( "XDG_DATA_HOME", QFile::encodeName( M_data.filePath( "home" ) ) );
          qputenv( "XDG_DATA_DIRS", QFile::encodeName( M_data.filePath( "system" ) ) );
          QCOMPARE( FigurePacks::userFolders().value( 0 ), M_data.filePath( "home/rcssmonitor/figures" ) );
          QVERIFY( FigurePacks::userFolders().contains( M_data.filePath( "system/rcssmonitor/figures" ) ) );
      }

    /*!
      the bundled packs: the Mannequin with its motion, and the Human with
      the Mannequin's motion (looked up: its model, its clips, its
      credits); their bodies as figure.json tells
    */
    void builtins()
      {
          const std::vector< FigurePack > & list = FigurePacks::builtins();
          QCOMPARE( int( list.size() ), 2 );
          const FigurePack * m = FigurePacks::builtin( "mannequin" );
          const FigurePack * h = FigurePacks::builtin( "human" );
          QVERIFY( m && h );
          QVERIFY( ! FigurePacks::builtin( "robot" ) );
          QCOMPARE( &list[0], m );
          QVERIFY2( m->ok(), qPrintable( m->error ) );
          QVERIFY2( h->ok(), qPrintable( h->error ) );
          QVERIFY( m->builtin && h->builtin );
          QCOMPARE( m->dir, QString( ":/figures/mannequin" ) );
          QCOMPARE( m->format, 1 );
          QCOMPARE( m->displayName(), QString( "Mannequin" ) );
          QCOMPARE( h->displayName(), QString( "Human" ) );

          // the Mannequin's motion, its own
          QCOMPARE( m->motion.pack, QString( "mannequin" ) );
          QCOMPARE( m->motion.model, QString( ":/figures/mannequin/Mannequin.glb" ) );
          QCOMPARE( m->motion.skeleton, QString( "quaternius" ) );
          QCOMPARE( m->motion.idle.clip, QString( "Idle_Loop" ) );
          QVERIFY( m->motion.idle.file.isEmpty() );
          QCOMPARE( m->motion.walk.clip, QString( "Walk_Loop" ) );
          QCOMPARE( m->motion.jog.clip, QString( "Jog_Fwd_Loop" ) );
          QCOMPARE( m->motion.sprint.clip, QString( "Sprint_Loop" ) );
          // the optional clips of 100STYLE: backward, to the right walking, to the left jogging
          QVERIFY( m->motion.walk_back.given );
          QCOMPARE( m->motion.walk_back.file, QString( ":/figures/mannequin/WalkBack.bvh" ) );
          QVERIFY( m->motion.walk_back.clip.isEmpty() );
          QCOMPARE( m->motion.jog_back.file, QString( ":/figures/mannequin/JogBack.bvh" ) );
          QCOMPARE( m->motion.walk_right.file, QString( ":/figures/mannequin/WalkRight.bvh" ) );
          QCOMPARE( m->motion.jog_left.file, QString( ":/figures/mannequin/JogLeft.bvh" ) );
          QVERIFY( ! m->motion.walk_left.given );
          QVERIFY( ! m->motion.jog_right.given );
          QCOMPARE( int( FigureMotionSpec::optionalKeys().size() ), 6 );
          QCOMPARE( QString( FigureMotionSpec::optionalKeys()[2] ), QString( "walk_left" ) );
          QCOMPARE( m->motion.optionalClips()[2], &m->motion.walk_left );
          QVERIFY( m->motion.credits.empty() );
          QCOMPARE( int( m->bodies.size() ), 1 );
          QCOMPARE( m->bodies[0].model, m->motion.model );
          QCOMPARE( m->bodies[0].team, QStringList( { "M_Main" } ) );
          QVERIFY( m->bodies[0].uniform.isEmpty() && m->bodies[0].hair.isEmpty() && m->bodies[0].hairstyles.isEmpty() );
          QCOMPARE( m->bodies[0].sex, FigureSex::Any );
          QVERIFY( ! m->hasSexes() );

          // the Human: the Mannequin's motion, and its credits
          QCOMPARE( h->motion.pack, QString( "mannequin" ) );
          QCOMPARE( h->motion.model, m->motion.model );
          QCOMPARE( h->motion.idle, m->motion.idle );
          QCOMPARE( h->motion.sprint, m->motion.sprint );
          QVERIFY( h->motion.credits == m->credits );
          QCOMPARE( int( h->bodies.size() ), 2 );
          const FigureBodySpec & man = h->bodies[0];
          const FigureBodySpec & woman = h->bodies[1];
          QCOMPARE( man.model, QString( ":/figures/human/BaseCharacterMale.glb" ) );
          QCOMPARE( woman.model, QString( ":/figures/human/BaseCharacterFemale.glb" ) );
          QCOMPARE( man.sex, FigureSex::Male );
          QCOMPARE( woman.sex, FigureSex::Female );
          QVERIFY( h->hasSexes() );
          for ( const FigureBodySpec & b : h->bodies )
          {
              QCOMPARE( b.uniform, QStringList( { "Body" } ) );
              QCOMPARE( b.hair, QStringList( { "Eyebrows" } ) );
              QVERIFY( b.team.isEmpty() );
          }
          QCOMPARE( man.hairstyles, QStringList( { "Hair_SimpleParted", "Hair_Buzzed" } ) );
          QCOMPARE( man.standard_hairstyle, 0 );
          QCOMPARE( int( man.extras.size() ), 1 );
          QCOMPARE( man.extras[0].material, QString( "Hair_Beard" ) );
          QCOMPARE( man.extras[0].chance, 0.3 );
          QCOMPARE( woman.hairstyles, QStringList( { "Hair_Buns", "Hair_Long", "Hair_BuzzedFemale" } ) );
          QCOMPARE( woman.standard_hairstyle, 2 );
          QVERIFY( woman.extras.empty() );
      }

    /*!
      the bits of the hairstyles and the extras, and the cast of the bodies
      as the looks are drawn from it (as test_figure_looks makes it)
    */
    void cast()
      {
          const FigurePack & h = *FigurePacks::builtin( "human" );
          const FigureBodySpec & man = h.bodies[0];
          QCOMPARE( man.bitOf( "Hair_SimpleParted" ), 1u );
          QCOMPARE( man.bitOf( "Hair_Buzzed" ), 2u );
          QCOMPARE( man.bitOf( "Hair_Beard" ), 4u );
          QCOMPARE( man.bitOf( "Eyebrows" ), 0u );
          QCOMPARE( man.bitOf( "Hair_Long" ), 0u );
          QCOMPARE( h.bodies[1].bitOf( "Hair_BuzzedFemale" ), 4u );

          const FigureCast c = h.cast();
          QCOMPARE( int( c.bodies.size() ), 2 );
          QCOMPARE( c.bodies[0].sex, FigureSex::Male );
          QCOMPARE( c.bodies[0].hairstyles, std::vector< std::uint32_t >( { 1, 2 } ) );
          QCOMPARE( c.bodies[0].standard_hair, 1u );
          QCOMPARE( int( c.bodies[0].extras.size() ), 1 );
          QCOMPARE( c.bodies[0].extras[0].bit, 4u );
          QCOMPARE( c.bodies[0].extras[0].chance, 0.3 );
          QCOMPARE( c.bodies[1].sex, FigureSex::Female );
          QCOMPARE( c.bodies[1].hairstyles, std::vector< std::uint32_t >( { 1, 2, 4 } ) );
          QCOMPARE( c.bodies[1].standard_hair, 4u );
          QVERIFY( c.bodies[1].extras.empty() );

          const FigureCast m = FigurePacks::builtin( "mannequin" )->cast();
          QCOMPARE( int( m.bodies.size() ), 1 );
          QCOMPARE( m.bodies[0].sex, FigureSex::Any );
          QVERIFY( m.bodies[0].hairstyles.empty() );
          QCOMPARE( m.bodies[0].standard_hair, 0u );
      }

    //! the credits: in short (each author and license once), in full with the URLs, the pack's and its motion's once each
    void credits()
      {
          const FigurePack & h = *FigurePacks::builtin( "human" );
          QCOMPARE( int( h.allCredits().size() ), 3 );
          QCOMPARE( h.creditLine(), QString( "Quaternius (CC0 1.0); Ian Mason (CC BY 4.0)" ) );
          QCOMPARE( h.creditText(),
                    QString( "Universal Base Characters [Standard] by Quaternius (CC0 1.0)\n"
                             "  https://quaternius.com/packs/universalbasecharacters.html\n"
                             "Universal Animation Library [Standard] by Quaternius (CC0 1.0)\n"
                             "  https://quaternius.com/packs/universalanimationlibrary.html\n"
                             "The 100STYLE Dataset (Neutral: a cycle each of walking and jogging backward and sideways, "
                             "cut, looped and thinned to 30 fps) by Ian Mason (CC BY 4.0)\n"
                             "  https://www.ianxmason.com/100style/" ) );
          const FigurePack & m = *FigurePacks::builtin( "mannequin" );
          QCOMPARE( int( m.allCredits().size() ), 2 );

          FigurePack p;
          p.credits = { FigureCredit{ "A", "X", "CC BY 4.0", QString() },
                        FigureCredit{ "Just a line", QString(), QString(), QString() },
                        FigureCredit{ "B", "X", "CC BY 4.0", "https://b" } };
          p.motion.credits = { FigureCredit{ "A", "X", "CC BY 4.0", QString() } }; // once
          QCOMPARE( int( p.allCredits().size() ), 3 );
          QCOMPARE( p.creditLine(), QString( "X (CC BY 4.0); Just a line" ) );
          QCOMPARE( p.creditText(), QString( "A by X (CC BY 4.0)\nJust a line\nB by X (CC BY 4.0)\n  https://b" ) );
          const FigureCredit author_only{ QString(), "X", QString(), QString() };
          const FigureCredit license_only{ QString(), QString(), "CC0", QString() };
          QCOMPARE( author_only.text(), QString( "X" ) );
          QCOMPARE( license_only.text(), QString( "CC0" ) );
      }

    /*!
      a figure.json read: a pack of its folder's name; the keys not known
      left alone (of a later version); the paints, the sexes, the
      hairstyles and the extras of the bodies; a credit as a line
    */
    void readPack()
      {
          QTemporaryDir folder;
          const QString dir = makePack( folder.path(), "robot",
                                        packJson( ", \"sex\": \"female\", \"skeleton\": \"quaternius\", \"later\": 1,"
                                                  " \"paint\": { \"M_Main\": \"uniform\", \"M_Joints\": \"hair\" },"
                                                  " \"hairstyles\": [ \"H1\", \"H2\" ], \"standard_hairstyle\": \"H2\","
                                                  " \"extras\": [ { \"material\": \"E1\", \"chance\": 0.25 }, { \"material\": \"E2\", \"chance\": 1 } ]",
                                                  "\"mannequin\"",
                                                  "\"credits\": [ \"A line\", { \"title\": \"T\", \"url\": \"https://t\" } ], \"later\": { \"x\": 1 }," ),
                                        { "Body.glb" } );
          const FigurePack p = FigurePack::read( dir, false );
          QVERIFY2( p.ok(), qPrintable( p.error ) );
          QCOMPARE( p.id, QString( "robot" ) );
          QCOMPARE( p.dir, dir );
          QVERIFY( ! p.builtin );
          QCOMPARE( p.name, QString( "A Pack" ) );
          QCOMPARE( p.motion.pack, QString( "mannequin" ) ); // looked up by scan()
          QVERIFY( p.motion.model.isEmpty() );
          QCOMPARE( int( p.credits.size() ), 2 );
          QCOMPARE( p.credits[0].title, QString( "A line" ) );
          QCOMPARE( p.credits[1].url, QString( "https://t" ) );
          const FigureBodySpec & b = p.bodies.at( 0 );
          QCOMPARE( b.model, QDir( dir ).filePath( "Body.glb" ) );
          QCOMPARE( b.skeleton, QString( "quaternius" ) );
          QCOMPARE( b.sex, FigureSex::Female );
          QCOMPARE( b.uniform, QStringList( { "M_Main" } ) );
          QCOMPARE( b.hair, QStringList( { "M_Joints" } ) );
          QVERIFY( b.team.isEmpty() );
          QCOMPARE( b.hairstyles, QStringList( { "H1", "H2" } ) );
          QCOMPARE( b.standard_hairstyle, 1 );
          QCOMPARE( b.bitOf( "E1" ), 4u );
          QCOMPARE( b.bitOf( "E2" ), 8u );
          QCOMPARE( b.extras[1].chance, 1.0 );
          QCOMPARE( p.cast().bodies[0].standard_hair, 2u );

          // a skeleton map, and the paints of the shades
          const QString mapped = makePack( folder.path(), "mapped",
                                           packJson( ", \"skeleton\": { \"hips\": \"pelvis\", \"head\": \"Head\" },"
                                                     " \"paint\": { \"M_Main\": \"skin\", \"M_Joints\": \"tint\" }" ),
                                           { "Body.glb" } );
          const FigurePack m = FigurePack::read( mapped, false );
          QVERIFY2( m.ok(), qPrintable( m.error ) );
          QVERIFY( m.bodies[0].skeleton.isEmpty() );
          QCOMPARE( int( m.bodies[0].bones.size() ), 2 );
          QCOMPARE( m.bodies[0].bones[0].first, QString( "head" ) ); // (in the order of the keys)
          QCOMPARE( m.bodies[0].bones[0].second, QString( "Head" ) );
          QCOMPARE( m.bodies[0].skin, QStringList( { "M_Main" } ) );
          QCOMPARE( m.bodies[0].tint, QStringList( { "M_Joints" } ) );

          // the clips: of the motion's model by their names, of files by
          // their names (by the extension), of a file by an object
          const QString clips = makePack( folder.path(), "clips",
                                          packJson( QByteArray(), "{ \"model\": \"Body.glb\", \"clips\": { \"idle\": \"Idle_Loop\","
                                                    " \"walk\": \"Walk.FBX\", \"jog\": { \"file\": \"Body.glb\", \"clip\": \"Jog_Fwd_Loop\" },"
                                                    " \"sprint\": { \"file\": \"Walk.FBX\" } } }" ),
                                          { "Body.glb", "Walk.FBX" } );
          const FigurePack c = FigurePack::read( clips, false );
          QVERIFY2( c.ok(), qPrintable( c.error ) );
          QCOMPARE( c.motion.idle, ( FigureClipSpec{ QString(), "Idle_Loop" } ) );
          QCOMPARE( c.motion.walk, ( FigureClipSpec{ QDir( clips ).filePath( "Walk.FBX" ), QString() } ) );
          QCOMPARE( c.motion.jog, ( FigureClipSpec{ QDir( clips ).filePath( "Body.glb" ), "Jog_Fwd_Loop" } ) );
          QCOMPARE( c.motion.sprint, ( FigureClipSpec{ QDir( clips ).filePath( "Walk.FBX" ), QString() } ) );
          QCOMPARE( c.motion.idle.text(), QString( "\"Idle_Loop\"" ) );
          QCOMPARE( c.motion.walk.text(), QString( "Walk.FBX" ) );
          QCOMPARE( c.motion.jog.text(), QString( "Body.glb:\"Jog_Fwd_Loop\"" ) );

          // no name: the folder's
          const QString unnamed = makePack( folder.path(), "unnamed",
                                            "{ \"format\": 1, \"motion\": \"mannequin\", \"bodies\": [ { \"model\": \"Body.glb\" } ] }",
                                            { "Body.glb" } );
          QCOMPARE( FigurePack::read( unnamed, false ).displayName(), QString( "unnamed" ) );
          QVERIFY( FigurePack::read( unnamed, false ).ok() );
      }

    //! what is wrong with a figure.json, told with where it is
    /*!
      the optional clips of the other ways: not told, none; told as the
      four, by a clip's name, a file, or an object; the keys of another
      pack's motion with it (the Human's, the Mannequin's)
    */
    void readOptional()
      {
          QTemporaryDir folder;
          const QString none = makePack( folder.path(), "none", packJson( QByteArray(), OWN_MOTION ), { "Body.glb", "Motion.glb" } );
          const FigurePack a = FigurePack::read( none, false );
          QVERIFY2( a.ok(), qPrintable( a.error ) );
          for ( const FigureClipSpec * c : a.motion.optionalClips() ) QVERIFY( ! c->given );
          QVERIFY( a.motion.walk.given );
          const QByteArray told = "{ \"model\": \"Motion.glb\", \"clips\": { \"idle\": \"Idle_Loop\", \"walk\": \"Walk_Loop\", "
              "\"jog\": \"Jog_Fwd_Loop\", \"sprint\": \"Sprint_Loop\", \"walk_back\": \"Back\", "
              "\"jog_left\": \"Side.glb\", \"walk_right\": { \"file\": \"Side.glb\", \"clip\": \"Right\" }, \"jog_right\": null } }";
          const QString dir = makePack( folder.path(), "told", packJson( QByteArray(), told ), { "Body.glb", "Motion.glb", "Side.glb" } );
          const FigurePack b = FigurePack::read( dir, false );
          QVERIFY2( b.ok(), qPrintable( b.error ) );
          QVERIFY( b.motion.walk_back.given );
          QCOMPARE( b.motion.walk_back.clip, QString( "Back" ) );
          QVERIFY( b.motion.walk_back.file.isEmpty() );
          QVERIFY( b.motion.jog_left.given );
          QCOMPARE( b.motion.jog_left.file, QDir( dir ).filePath( "Side.glb" ) );
          QCOMPARE( b.motion.walk_right.file, QDir( dir ).filePath( "Side.glb" ) );
          QCOMPARE( b.motion.walk_right.clip, QString( "Right" ) );
          QVERIFY( ! b.motion.jog_back.given );
          QVERIFY( ! b.motion.walk_left.given );
          QVERIFY( ! b.motion.jog_right.given ); // null: not told
          QVERIFY( ! ( b.motion.jog_back == b.motion.walk_back ) );
          // another pack's motion: its clips with it
          const FigurePack & human = *FigurePacks::builtin( "human" );
          const FigurePack & mannequin = *FigurePacks::builtin( "mannequin" );
          for ( std::size_t k = 0; k < human.motion.optionalClips().size(); ++k )
          {
              QVERIFY( *human.motion.optionalClips()[k] == *mannequin.motion.optionalClips()[k] );
          }
      }

    void readErrors_data()
      {
          QTest::addColumn< QByteArray >( "json" );
          QTest::addColumn< QString >( "error" );
          const QByteArray clips = "\"clips\": { \"idle\": \"I\", \"walk\": \"W\", \"jog\": \"J\", \"sprint\": \"S\" }";
          QTest::newRow( "not json" ) << QByteArray( "{ \"format\": 1,\n  \"name\": }" ) << QString( "figure.json: " );
          QTest::newRow( "not json: where" ) << QByteArray( "{ \"format\": 1,\n  \"name\": }" ) << QString( "(line 2, column " );
          QTest::newRow( "not an object" ) << QByteArray( "[ 1 ]" ) << QString( "figure.json: not an object" );
          QTest::newRow( "no format" ) << QByteArray( "{ \"motion\": \"mannequin\" }" ) << QString( "no \"format\" (1)" );
          QTest::newRow( "format string" ) << QByteArray( "{ \"format\": \"1\" }" ) << QString( "no \"format\" (1)" );
          QTest::newRow( "format 1.5" ) << QByteArray( "{ \"format\": 1.5 }" ) << QString( "no \"format\" (1)" );
          QTest::newRow( "format later" ) << QByteArray( "{ \"format\": 2 }" ) << QString( "format: 2, but this rcssmonitor reads 1" );
          QTest::newRow( "format 0" ) << QByteArray( "{ \"format\": 0 }" ) << QString( "format: 0, but this rcssmonitor reads 1" );
          QTest::newRow( "name" ) << packJson( QByteArray(), "\"mannequin\"", "\"name\": 3," ) << QString( "name: not a string" );
          QTest::newRow( "credits" ) << packJson( QByteArray(), "\"mannequin\"", "\"credits\": 3," ) << QString( "credits: not a list" );
          QTest::newRow( "credit" ) << packJson( QByteArray(), "\"mannequin\"", "\"credits\": [ \"a\", 3 ]," ) << QString( "credits[1]: neither an object nor a string" );
          QTest::newRow( "credit url" ) << packJson( QByteArray(), "\"mannequin\"", "\"credits\": [ { \"url\": 3 } ]," ) << QString( "credits[0].url: not a string" );
          QTest::newRow( "no motion" ) << QByteArray( "{ \"format\": 1, \"bodies\": [ { \"model\": \"Body.glb\" } ] }" ) << QString( "no \"motion\"" );
          QTest::newRow( "motion number" ) << packJson( QByteArray(), "3" ) << QString( "no \"motion\"" );
          QTest::newRow( "motion empty" ) << packJson( QByteArray(), "\"\"" ) << QString( "motion: empty" );
          QTest::newRow( "motion itself" ) << packJson( QByteArray(), "\"pack\"" ) << QString( "motion: the pack itself" );
          QTest::newRow( "motion no model" ) << packJson( QByteArray(), "{ " + clips + " }" ) << QString( "motion: no \"model\"" );
          QTest::newRow( "motion model" ) << packJson( QByteArray(), "{ \"model\": \"None.glb\", " + clips + " }" ) << QString( "motion.model: \"None.glb\" is not found in the pack's folder" );
          QTest::newRow( "motion skeleton" ) << packJson( QByteArray(), "{ \"model\": \"Body.glb\", \"skeleton\": \"biped\", " + clips + " }" ) << QString( "motion.skeleton: \"biped\" is not one of quaternius, mixamo, vroid, xsens, vrm, nor a map of the roles to the bones" );
          QTest::newRow( "motion no clips" ) << packJson( QByteArray(), "{ \"model\": \"Body.glb\" }" ) << QString( "motion: no \"clips\"" );
          QTest::newRow( "motion no jog" ) << packJson( QByteArray(), "{ \"model\": \"Body.glb\", \"clips\": { \"idle\": \"I\", \"walk\": \"W\", \"sprint\": \"S\" } }" ) << QString( "motion.clips: no \"jog\"" );
          const auto clip = []( const QByteArray & walk ) {
              return packJson( QByteArray(), "{ \"model\": \"Body.glb\", \"clips\": { \"idle\": \"I\", \"walk\": " + walk
                               + ", \"jog\": \"J\", \"sprint\": \"S\" } }" );
          };
          QTest::newRow( "clip number" ) << clip( "3" ) << QString( "motion.clips.walk: neither a clip's name, a file, nor an object of a \"file\" and a \"clip\"" );
          QTest::newRow( "clip empty" ) << clip( "\"\"" ) << QString( "motion.clips.walk: empty" );
          QTest::newRow( "clip file" ) << clip( "\"Walk.fbx\"" ) << QString( "motion.clips.walk: \"Walk.fbx\" is not found in the pack's folder" );
          QTest::newRow( "clip file case" ) << clip( "\"Walk.GLTF\"" ) << QString( "motion.clips.walk: \"Walk.GLTF\" is not found in the pack's folder" );
          QTest::newRow( "clip file bvh" ) << clip( "\"Walk.bvh\"" ) << QString( "motion.clips.walk: \"Walk.bvh\" is not found in the pack's folder" );
          QTest::newRow( "clip file vrma" ) << clip( "\"Walk.vrma\"" ) << QString( "motion.clips.walk: \"Walk.vrma\" is not found in the pack's folder" );
          QTest::newRow( "clip object no file" ) << clip( "{ \"clip\": \"W\" }" ) << QString( "motion.clips.walk: no \"file\"" );
          QTest::newRow( "clip object file" ) << clip( "{ \"file\": \"Walk.vrm\" }" ) << QString( "motion.clips.walk.file: \"Walk.vrm\" is not found in the pack's folder" );
          QTest::newRow( "clip object clip" ) << clip( "{ \"file\": \"Body.glb\", \"clip\": 3 }" ) << QString( "motion.clips.walk.clip: not a string" );
          const auto optional = []( const QByteArray & key,
                                    const QByteArray & value ) {
              return packJson( QByteArray(), "{ \"model\": \"Body.glb\", \"clips\": { \"idle\": \"I\", \"walk\": \"W\", "
                               "\"jog\": \"J\", \"sprint\": \"S\", \"" + key + "\": " + value + " } }" );
          };
          QTest::newRow( "walk_back empty" ) << optional( "walk_back", "\"\"" ) << QString( "motion.clips.walk_back: empty" );
          QTest::newRow( "jog_left number" ) << optional( "jog_left", "3" ) << QString( "motion.clips.jog_left: neither a clip's name" );
          QTest::newRow( "walk_right file" ) << optional( "walk_right", "\"Side.bvh\"" ) << QString( "motion.clips.walk_right: \"Side.bvh\" is not found in the pack's folder" );
          QTest::newRow( "jog_back no file" ) << optional( "jog_back", "{ \"clip\": \"B\" }" ) << QString( "motion.clips.jog_back: no \"file\"" );
          QTest::newRow( "no bodies" ) << QByteArray( "{ \"format\": 1, \"motion\": \"mannequin\" }" ) << QString( "no \"bodies\"" );
          QTest::newRow( "bodies empty" ) << QByteArray( "{ \"format\": 1, \"motion\": \"mannequin\", \"bodies\": [] }" ) << QString( "no \"bodies\"" );
          QTest::newRow( "body not object" ) << QByteArray( "{ \"format\": 1, \"motion\": \"mannequin\", \"bodies\": [ 3 ] }" ) << QString( "bodies[0]: not an object" );
          QTest::newRow( "body no model" ) << QByteArray( "{ \"format\": 1, \"motion\": \"mannequin\", \"bodies\": [ { } ] }" ) << QString( "bodies[0]: no \"model\"" );
          QTest::newRow( "body model" ) << QByteArray( "{ \"format\": 1, \"motion\": \"mannequin\", \"bodies\": [ { \"model\": \"Body.glb\" }, { \"model\": \"x/None.glb\" } ] }" ) << QString( "bodies[1].model: \"x/None.glb\" is not found in the pack's folder" );
          QTest::newRow( "body model empty" ) << QByteArray( "{ \"format\": 1, \"motion\": \"mannequin\", \"bodies\": [ { \"model\": \"\" } ] }" ) << QString( "bodies[0].model: empty" );
          QTest::newRow( "sex" ) << packJson( ", \"sex\": \"other\"" ) << QString( "bodies[0].sex: \"other\" is not male or female" );
          QTest::newRow( "paint" ) << packJson( ", \"paint\": { \"M_Main\": \"unifrom\" }" ) << QString( "bodies[0].paint: \"M_Main\": \"unifrom\" is not one of team, tint, uniform, skin, hair, model" );
          QTest::newRow( "paint object" ) << packJson( ", \"paint\": 3" ) << QString( "bodies[0].paint: not an object" );
          QTest::newRow( "paint hairstyle" ) << packJson( ", \"hairstyles\": [ \"H\" ], \"paint\": { \"H\": \"team\" }" ) << QString( "bodies[0].paint: \"H\" is a hairstyle or an extra, in the hair's color" );
          QTest::newRow( "hairstyles twice" ) << packJson( ", \"hairstyles\": [ \"H\", \"H\" ]" ) << QString( "bodies[0].hairstyles: \"H\" is listed twice" );
          QTest::newRow( "hairstyle name" ) << packJson( ", \"hairstyles\": [ 3 ]" ) << QString( "bodies[0].hairstyles[0]: not a name" );
          QTest::newRow( "hairstyles list" ) << packJson( ", \"hairstyles\": \"H\"" ) << QString( "bodies[0].hairstyles: not a list" );
          QTest::newRow( "standard hairstyle" ) << packJson( ", \"hairstyles\": [ \"H\" ], \"standard_hairstyle\": \"G\"" ) << QString( "bodies[0].standard_hairstyle: \"G\" is not one of the hairstyles" );
          QTest::newRow( "extras list" ) << packJson( ", \"extras\": 3" ) << QString( "bodies[0].extras: not a list" );
          QTest::newRow( "extra material" ) << packJson( ", \"extras\": [ { \"chance\": 0.5 } ]" ) << QString( "bodies[0].extras[0]: no \"material\"" );
          QTest::newRow( "extra chance" ) << packJson( ", \"extras\": [ { \"material\": \"E\", \"chance\": 1.5 } ]" ) << QString( "bodies[0].extras[0]: no \"chance\" from 0 to 1" );
          QTest::newRow( "extra no chance" ) << packJson( ", \"extras\": [ { \"material\": \"E\" } ]" ) << QString( "bodies[0].extras[0]: no \"chance\" from 0 to 1" );
          QTest::newRow( "extra twice" ) << packJson( ", \"hairstyles\": [ \"H\" ], \"extras\": [ { \"material\": \"H\", \"chance\": 0.5 } ]" ) << QString( "bodies[0].extras[0]: \"H\" is listed twice" );
          QTest::newRow( "too many hairs" ) << packJson( ", \"hairstyles\": [ \"A\", \"B\", \"C\" ], \"extras\": [ { \"material\": \"D\", \"chance\": 0.5 }, { \"material\": \"E\", \"chance\": 0.5 } ]" ) << QString( "bodies[0]: more than 4 hairstyles and extras" );
          QTest::newRow( "skeleton" ) << packJson( ", \"skeleton\": \"biped\"" ) << QString( "bodies[0].skeleton: \"biped\" is not one of quaternius, mixamo, vroid, xsens, vrm" );
          QTest::newRow( "skeleton role" ) << packJson( ", \"skeleton\": { \"hips\": \"pelvis\", \"lefthand\": \"hand_l\" }" ) << QString( "bodies[0].skeleton: \"lefthand\" is not one of hips, spine, chest, upperChest, neck, head, leftShoulder, leftUpperArm, leftLowerArm, leftHand," );
          QTest::newRow( "skeleton bone" ) << packJson( ", \"skeleton\": { \"hips\": 3 }" ) << QString( "bodies[0].skeleton.hips: not a bone's name" );
          QTest::newRow( "skeleton empty" ) << packJson( ", \"skeleton\": { }" ) << QString( "bodies[0].skeleton: empty" );
          QTest::newRow( "motion skeleton map" ) << packJson( QByteArray(), "{ \"model\": \"Body.glb\", \"skeleton\": { \"knee\": \"calf_l\" }, " + clips + " }" ) << QString( "motion.skeleton: \"knee\" is not one of hips" );
      }

    void readErrors()
      {
          QFETCH( QByteArray, json );
          QFETCH( QString, error );
          QTemporaryDir folder;
          const QString dir = makePack( folder.path(), "pack", json, { "Body.glb" } );
          const FigurePack p = FigurePack::read( dir, false );
          QVERIFY( ! p.ok() );
          QVERIFY2( p.error.contains( error ), qPrintable( p.error ) );
      }

    //! a folder without figure.json is not a pack
    void noFile()
      {
          QTemporaryDir folder;
          const FigurePack p = FigurePack::read( folder.path(), false );
          QCOMPARE( p.error, QString( "no figure.json" ) );
          QCOMPARE( p.id, QDir( folder.path() ).dirName() );
      }

    /*!
      the packs looked for: the bundled ones, the folder's (in the order of
      their names), the user's; the first of a name the one; the name of
      the cylinders taken; a folder without figure.json told; the motions
      of other packs looked up, through them, or told why not; a folder
      that is a pack itself
    */
    void scan()
      {
          QTemporaryDir folder;
          const QString f = folder.path();
          makePack( f, "robot", packJson( QByteArray(), OWN_MOTION ), { "Body.glb", "Motion.glb" } );
          makePack( f, "uses_robot", packJson( QByteArray(), "\"robot\"" ), { "Body.glb" } );
          makePack( f, "through", packJson( QByteArray(), "\"uses_robot\"" ), { "Body.glb" } );
          makePack( f, "mannequin", packJson(), { "Body.glb" } );
          makePack( f, "simple", packJson(), { "Body.glb" } );
          makePack( f, "nobody", packJson( QByteArray(), "\"nobody_at_all\"" ), { "Body.glb" } );
          makePack( f, "broken", "{ \"format\": 3 }" );
          makePack( f, "uses_broken", packJson( QByteArray(), "\"broken\"" ), { "Body.glb" } );
          makePack( f, "ping", packJson( QByteArray(), "\"pong\"" ), { "Body.glb" } );
          makePack( f, "pong", packJson( QByteArray(), "\"ping\"" ), { "Body.glb" } );
          QDir( f ).mkpath( "empty" );
          // the user's folder: one more, and one of a name of the folder's
          const QString user = FigurePacks::userFolders().value( 0 );
          makePack( user, "xdg", packJson(), { "Body.glb" } );
          makePack( user, "robot", packJson(), { "Body.glb" } );

          const std::vector< FigurePack > list = FigurePacks::scan( f );
          QStringList ids;
          for ( const FigurePack & p : list ) ids << p.id;
          QCOMPARE( ids, QStringList( { "mannequin", "human",
                                        "broken", "empty", "mannequin", "nobody", "ping", "pong", "robot",
                                        "simple", "through", "uses_broken", "uses_robot",
                                        "robot", "xdg" } ) );
          const auto error = [&list]( const int i ) { return list[i].error; };
          QVERIFY( list[0].ok() && list[0].builtin );
          QVERIFY( list[1].ok() && list[1].builtin );
          QVERIFY2( error( 2 ).startsWith( "format: 3" ), qPrintable( error( 2 ) ) );
          QCOMPARE( error( 3 ), QString( "no figure.json" ) );
          QCOMPARE( error( 4 ), QString( "another pack has the same name: :/figures/mannequin" ) );
          QCOMPARE( error( 5 ), QString( "motion: no pack \"nobody_at_all\"" ) );
          QCOMPARE( error( 6 ), QString( "motion: the packs' motions refer to each other" ) );
          QCOMPARE( error( 7 ), QString( "motion: the packs' motions refer to each other" ) );
          QVERIFY2( list[8].ok(), qPrintable( error( 8 ) ) );
          QCOMPARE( error( 9 ), QString( "\"simple\" is the name of the cylinders" ) );
          QVERIFY2( list[10].ok(), qPrintable( error( 10 ) ) );
          QVERIFY2( error( 11 ).startsWith( "motion: the pack \"broken\" cannot be read: format: 3" ), qPrintable( error( 11 ) ) );
          QVERIFY2( list[12].ok(), qPrintable( error( 12 ) ) );
          QCOMPARE( error( 13 ), "another pack has the same name: " + QDir( f ).filePath( "robot" ) );
          QVERIFY2( list[14].ok(), qPrintable( error( 14 ) ) );
          QCOMPARE( list[14].dir, QDir( user ).filePath( "xdg" ) );

          // the motions: its own; another pack's, through one more
          const FigurePack & robot = list[8];
          QCOMPARE( robot.motion.pack, QString( "robot" ) );
          QCOMPARE( robot.motion.model, QDir( f ).filePath( "robot/Motion.glb" ) );
          for ( const int i : { 10, 12 } )
          {
              QCOMPARE( list[i].motion.pack, QString( "robot" ) );
              QCOMPARE( list[i].motion.model, robot.motion.model );
              QCOMPARE( list[i].motion.walk.clip, QString( "Walk_Loop" ) );
          }
          QCOMPARE( list[14].motion.model, QString( ":/figures/mannequin/Mannequin.glb" ) );

          // a folder that is a pack itself
          const std::vector< FigurePack > one = FigurePacks::scan( QDir( f ).filePath( "robot" ) );
          QVERIFY( byId( one, "robot" ) );
          QCOMPARE( byId( one, "robot" )->dir, QDir( f ).filePath( "robot" ) );
          QVERIFY( byId( one, "robot" )->ok() );
          QVERIFY( ! byId( one, "xdg" ) || byId( one, "xdg" )->ok() );
          // the user's folder chosen as the folder: once
          const std::vector< FigurePack > users = FigurePacks::scan( user );
          QCOMPARE( int( std::count_if( users.begin(), users.end(), []( const FigurePack & p ) { return p.id == "xdg"; } ) ), 1 );
          QDir( user ).removeRecursively();
      }

    /*!
      the packs looked for once: again when the folder changes, or after
      refresh() (counted by generation()); why a pack could not be drawn,
      kept until told otherwise
    */
    void cache()
      {
          QTemporaryDir a, b;
          makePack( a.path(), "one", packJson(), { "Body.glb" } );
          FigurePacks::refresh();
          const std::vector< FigurePack > * list = &FigurePacks::packs( a.path() );
          const int g = FigurePacks::generation();
          QVERIFY( FigurePacks::find( "one", a.path() ) );
          QVERIFY( FigurePacks::find( "mannequin", a.path() )->builtin );
          QVERIFY( ! FigurePacks::find( "two", a.path() ) );
          QCOMPARE( FigurePacks::generation(), g ); // not looked for again
          QCOMPARE( &FigurePacks::packs( a.path() ), list );

          makePack( a.path(), "two", packJson(), { "Body.glb" } );
          QVERIFY( ! FigurePacks::find( "two", a.path() ) ); // not yet
          FigurePacks::refresh();
          QVERIFY( FigurePacks::find( "two", a.path() ) );
          QCOMPARE( FigurePacks::generation(), g + 1 );
          QVERIFY( ! FigurePacks::find( "one", b.path() ) ); // another folder: looked for again
          QCOMPARE( FigurePacks::generation(), g + 2 );
          QVERIFY( FigurePacks::find( "one", a.path() ) );

          QVERIFY( FigurePacks::drawError( "one" ).isEmpty() );
          FigurePacks::noteError( "one", "why" );
          QCOMPARE( FigurePacks::drawError( "one" ), QString( "why" ) );
          FigurePacks::refresh();
          QCOMPARE( FigurePacks::drawError( "one" ), QString( "why" ) ); // kept, the pack tried again
          FigurePacks::noteError( "one", QString() );
          QVERIFY( FigurePacks::drawError( "one" ).isEmpty() );
      }

    /*!
      a pack of FBX files, as of Mixamo: the body and the motion's model an
      FBX file (its unit and axes converted, its roles by Mixamo's names),
      the walk a file of a skeleton without a skin (its rest as saved, taken
      as the motion's), not in place (put in place), the other clips of
      another file and skeleton (the Mannequin's glTF; its bones not of the
      motion's skeleton, found by their own names): the figure moves as
      the Mannequin does, at the same speeds; a clip of the motion's own
      model not in place is put in place too; what is wrong, told
    */
    void fbxPacks()
      {
          QTemporaryDir folder;
          const QString dir = QDir( folder.path() ).filePath( "mixamo" );
          QVERIFY( QDir().mkpath( dir ) );
          const QString data = QStringLiteral( TEST_SOURCE_DIR "/test/data/fbx/" );
          QVERIFY( QFile::copy( data + "figure.fbx", dir + "/Character.fbx" ) );
          QVERIFY( QFile::copy( data + "walk.fbx", dir + "/Walking.fbx" ) );
          QVERIFY( QFile::copy( ":/figures/mannequin/Mannequin.glb", dir + "/Mannequin.glb" ) );
          const auto json = []( const QByteArray & idle, const QByteArray & walk ) {
              return "{ \"format\": 1, \"motion\": { \"model\": \"Character.fbx\", \"skeleton\": \"mixamo\", \"clips\": {\n"
                  "  \"idle\": " + idle + ", \"walk\": " + walk + ",\n"
                  "  \"jog\": { \"file\": \"Mannequin.glb\", \"clip\": \"Jog_Fwd_Loop\" },\n"
                  "  \"sprint\": { \"file\": \"Mannequin.glb\", \"clip\": \"Sprint_Loop\" } } },\n"
                  "  \"bodies\": [ { \"model\": \"Character.fbx\", \"paint\": { \"M_Main\": \"team\" } } ] }\n";
          };
          const QByteArray idle = "{ \"file\": \"Mannequin.glb\", \"clip\": \"Idle_Loop\" }";
          QVERIFY( writeFile( dir + "/figure.json", json( idle, "\"Walking.fbx\"" ) ) );
          const FigurePack pack = FigurePack::read( dir, false );
          QVERIFY2( pack.ok(), qPrintable( pack.error ) );

          Figures3D mannequin;
          QVERIFY( mannequin.load() );
          Figures3D figures;
          QVERIFY( figures.load() );
          QVERIFY2( figures.setPack( pack ), qPrintable( figures.error() ) );
          QCOMPARE( figures.model().images.size(), std::size_t( 1 ) ); // the texture embedded
          QCOMPARE( figures.paints()[0].kind, int( FigureUniform::Paint::Figure ) );
          // the same speeds and strides as the Mannequin's clips
          QCOMPARE( figures.gaitClips().size(), mannequin.gaitClips().size() );
          for ( std::size_t k = 0; k < figures.gaitClips().size(); ++k )
          {
              const GaitClip & got = figures.gaitClips()[k];
              const GaitClip & want = mannequin.gaitClips()[k];
              QVERIFY2( std::fabs( got.speed / want.speed - 1.0f ) < 0.02f,
                        qPrintable( QString( "%1: %2 %3" ).arg( k ).arg( got.speed ).arg( want.speed ) ) );
              QVERIFY( std::fabs( got.duration - want.duration ) < 0.02f );
          }
          // walking: its limbs as the Mannequin's
          const auto limbOf = []( const Figures3D & f, const FigureInstance & fi, const double time,
                                  const HumanBone upper, const HumanBone lower ) {
              std::vector< QMatrix4x4 > world;
              f.worldNodes( fi, time, world );
              const HumanoidRig & rig = f.rig( f.bodyOf( fi ) );
              return ( world[rig.node( lower )].column( 3 ) - world[rig.node( upper )].column( 3 ) ).toVector3D().normalized();
          };
          FigureInstance fi{ 0, 0.0f, 0.0f, 0.0f, 1.85f, QVector4D( 1.0f, 0.0f, 0.0f, 1.0f ) };
          fi.motion.vx = 1.0f; // walking (1 m/s, ahead)
          for ( const double time : { 0.0, 0.3, 0.9 } )
          {
              fi.motion.gait = time * 0.75;
              for ( const auto & limb : { std::make_pair( HumanBone::LeftUpperLeg, HumanBone::LeftLowerLeg ),
                                          std::make_pair( HumanBone::RightLowerLeg, HumanBone::RightFoot ),
                                          std::make_pair( HumanBone::LeftUpperArm, HumanBone::LeftLowerArm ) } )
              {
                  const float dot = QVector3D::dotProduct( limbOf( figures, fi, time, limb.first, limb.second ),
                                                           limbOf( mannequin, fi, time, limb.first, limb.second ) );
                  QVERIFY2( dot > 0.995f, qPrintable( QString( "%1 %2: %3" ).arg( time ).arg( humanBoneName( limb.first ) ).arg( dot ) ) );
              }
          }

          // the template of a Mixamo pack (docs/figure_packs/mixamo), with these files as Mixamo's
          {
              const QString mixamo = QDir( folder.path() ).filePath( "template" );
              QVERIFY( QDir().mkpath( mixamo ) );
              QVERIFY( QFile::copy( QStringLiteral( TEST_SOURCE_DIR "/docs/figure_packs/mixamo/figure.json" ), mixamo + "/figure.json" ) );
              QVERIFY( QFile::copy( data + "figure.fbx", mixamo + "/Character.fbx" ) );
              for ( const char * clip : { "idle.fbx", "walk.fbx", "jog.fbx", "sprint.fbx" } )
              {
                  QVERIFY( QFile::copy( data + "walk.fbx", mixamo + "/" + clip ) );
              }
              const FigurePack t = FigurePack::read( mixamo, false );
              QVERIFY2( t.ok(), qPrintable( t.error ) );
              Figures3D templated;
              QVERIFY( templated.load() );
              QVERIFY2( templated.setPack( t ), qPrintable( templated.error() ) );
          }

          // the motion's own model with a clip not in place: in place
          {
              TestGltf::File f = TestGltf::fromModel( TestGltf::mannequin() );
              const GltfModel & glb = TestGltf::mannequin();
              const int root = glb.nodeIndex( "root" );
              const int walk = glb.clipIndex( "Walk_Loop" );
              TestGltf::Animation & a = f.animations[walk];
              const float duration = glb.clips[walk].duration;
              a.channels.erase( std::remove_if( a.channels.begin(), a.channels.end(),
                                                [root]( const TestGltf::Animation::Channel & c ) { return c.node == root && c.path == "translation"; } ),
                                a.channels.end() );
              // the root's keys sampled, a stride ahead (+z of the model, in its parent's frame) added in a line
              std::vector< QMatrix4x4 > global;
              glb.globals( glb.rest, global );
              const int above = glb.parent[root];
              const QVector3D ahead = ( above >= 0 ? global[above].inverted().mapVector( QVector3D( 0.0f, 0.0f, 1.28f ) )
                                        : QVector3D( 0.0f, 0.0f, 1.28f ) );
              TestGltf::Animation::Channel forward{ root, "translation", {}, {}, false };
              for ( int k = 0; k <= 40; ++k )
              {
                  const float t = duration * k / 40;
                  std::vector< NodeTransform > local = glb.rest;
                  glb.sample( glb.clips[walk], std::min( t, duration - 1.0e-4f ), local );
                  const QVector3D at = local[root].t + ahead * ( float( k ) / 40 );
                  forward.times.push_back( t );
                  forward.values.insert( forward.values.end(), { at.x(), at.y(), at.z() } );
              }
              a.channels.push_back( forward );
              const QString own = QDir( folder.path() ).filePath( "own" );
              QVERIFY( QDir().mkpath( own ) );
              QVERIFY( TestGltf::File::write( own + "/Forward.glb", f.glb() ) );
              QVERIFY( writeFile( own + "/figure.json", "{ \"format\": 1, \"motion\": { \"model\": \"Forward.glb\", \"clips\": {"
                                  " \"idle\": \"Idle_Loop\", \"walk\": \"Walk_Loop\", \"jog\": \"Jog_Fwd_Loop\", \"sprint\": \"Sprint_Loop\" } },"
                                  " \"bodies\": [ { \"model\": \"Forward.glb\" } ] }" ) );
              Figures3D forwards;
              QVERIFY( forwards.load() );
              QVERIFY2( forwards.setPack( FigurePack::read( own, false ) ), qPrintable( forwards.error() ) );
              const GltfModel & m = forwards.motionModel();
              QVERIFY( clipDrift( m, forwards.motionRig(), m.clips[m.clipIndex( "Walk_Loop" )] ) < 1.0e-3f );

              // a motion's model of a skeleton only (no mesh; glTF, its rest at rest), the body another file
              TestGltf::File skeleton = f;
              skeleton.meshes.clear();
              skeleton.materials.clear();
              skeleton.skins.clear();
              QVERIFY( TestGltf::File::write( own + "/Skeleton.glb", skeleton.glb() ) );
              QVERIFY( QFile::copy( ":/figures/mannequin/Mannequin.glb", own + "/Mannequin.glb" ) );
              QVERIFY( writeFile( own + "/figure.json", "{ \"format\": 1, \"motion\": { \"model\": \"Skeleton.glb\", \"clips\": {"
                                  " \"idle\": \"Idle_Loop\", \"walk\": \"Walk_Loop\", \"jog\": \"Jog_Fwd_Loop\", \"sprint\": \"Sprint_Loop\" } },"
                                  " \"bodies\": [ { \"model\": \"Mannequin.glb\" } ] }" ) );
              Figures3D bare;
              QVERIFY( bare.load() );
              QVERIFY2( bare.setPack( FigurePack::read( own, false ) ), qPrintable( bare.error() ) );
              QVERIFY( bare.motionModel().vertices.empty() );
              QVERIFY( std::fabs( bare.gaitClips()[0].speed / mannequin.gaitClips()[0].speed - 1.0f ) < 0.02f );
              for ( std::size_t k = 0; k < forwards.gaitClips().size(); ++k )
              {
                  QVERIFY2( std::fabs( forwards.gaitClips()[k].speed / mannequin.gaitClips()[k].speed - 1.0f ) < 0.02f,
                            qPrintable( QString( "%1: %2 %3" ).arg( k ).arg( forwards.gaitClips()[k].speed ).arg( mannequin.gaitClips()[k].speed ) ) );
              }
          }

          // what is wrong: a file of no clips, a clip not in a file, a file not of FBX, a clip not of walking
          const auto why = [&]( const QByteArray & walk ) {
              writeFile( dir + "/figure.json", json( idle, walk ) );
              Figures3D f;
              f.load();
              const FigurePack p = FigurePack::read( dir, false );
              if ( ! p.ok() ) return p.error;
              return f.setPack( p ) ? QString() : f.error();
          };
          QVERIFY( writeFile( dir + "/Text.fbx", "not an FBX file" ) );
          QCOMPARE( why( "\"Character.fbx\"" ), QString( "mixamo: Character.fbx: no clips" ) );
          QCOMPARE( why( "{ \"file\": \"Walking.fbx\", \"clip\": \"Run\" }" ), QString( "mixamo: Walking.fbx: no clip \"Run\" (its clips: \"Scene\")" ) );
          QVERIFY2( why( "\"Text.fbx\"" ).startsWith( "mixamo: Text.fbx: not an FBX file (" ), qPrintable( why( "\"Text.fbx\"" ) ) );
          QCOMPARE( why( idle ), QString( "mixamo: the clip Mannequin.glb:\"Idle_Loop\" is not of walking or running (the feet do not come down and go back)" ) );
      }

    /*!
      clips of BVH and VRM Animation files: BVH files written z up in
      centimeters as captured (the root's OFFSET at the origin, the motion
      across a room going another way: settled), or -x up facing back
      (stood upright and turned to face +z), and a VRMA file, mixed with the
      Mannequin's clips: the same speeds, limbs and feet as the Mannequin's. A BVH file as the motion's model
      itself (a skeleton without a mesh, its own clip), with clips of files
      written -z, -y and +x up. The walk as Blender's BVH exporter writes it
    */
    void motionFiles()
      {
          QTemporaryDir folder;
          const QString dir = QDir( folder.path() ).filePath( "capture" );
          QVERIFY( QDir().mkpath( dir ) );
          QVERIFY( QFile::copy( ":/figures/mannequin/Mannequin.glb", dir + "/Mannequin.glb" ) );
          const GltfModel & glb = TestGltf::mannequin();
          HumanoidRig glb_rig;
          QVERIFY( glb_rig.build( glb, quaterniusBoneNames() ) );
          TestBvh::Options zup; // z up, in centimeters, turned, as captured (the root's OFFSET at the origin, the motion across a room, going another way)
          zup.z_up = true;
          zup.scale = 100.0f;
          zup.facing = 30.0f;
          zup.origin_root = true;
          zup.room = QVector3D( 3.0f, 0.0f, -2.0f );
          zup.heading = 70.0f;
          QVERIFY( writeFile( dir + "/Walk.bvh", TestBvh::bvhOf( glb, &glb.clips[glb.clipIndex( "Walk_Loop" )], zup ) ) );
          TestBvh::Options turned; // up -x, facing back
          turned.facing = 180.0f;
          turned.turn = QQuaternion::fromAxisAndAngle( 0.0f, 0.0f, 1.0f, 90.0f );
          turned.order = "XYZ";
          QVERIFY( writeFile( dir + "/Sprint.bvh", TestBvh::bvhOf( glb, &glb.clips[glb.clipIndex( "Sprint_Loop" )], turned ) ) );
          // the other ways up: -z, -y (upside down), +x
          const auto other = [&]( const char * name, const char * clip, const QQuaternion & turn ) {
              TestBvh::Options o;
              o.turn = turn;
              return writeFile( dir + "/" + name, TestBvh::bvhOf( glb, &glb.clips[glb.clipIndex( clip )], o ) );
          };
          QVERIFY( other( "WalkUnder.bvh", "Walk_Loop", QQuaternion::fromAxisAndAngle( 1.0f, 0.0f, 0.0f, -90.0f ) ) );
          QVERIFY( other( "JogDown.bvh", "Jog_Fwd_Loop", QQuaternion::fromAxisAndAngle( 1.0f, 0.0f, 0.0f, 180.0f ) ) );
          QVERIFY( other( "SprintSide.bvh", "Sprint_Loop", QQuaternion::fromAxisAndAngle( 0.0f, 0.0f, 1.0f, -90.0f ) ) );
          QVERIFY( TestGltf::File::write( dir + "/Jog.vrma", TestGltf::animationOf( glb, glb_rig, "Jog_Fwd_Loop" ).glb() ) );
          QVERIFY( writeFile( dir + "/figure.json",
                              "{ \"format\": 1, \"motion\": { \"model\": \"Mannequin.glb\", \"skeleton\": \"quaternius\", \"clips\": {\n"
                              "  \"idle\": \"Idle_Loop\", \"walk\": \"Walk.bvh\", \"jog\": \"Jog.vrma\", \"sprint\": \"Sprint.bvh\" } },\n"
                              "  \"bodies\": [ { \"model\": \"Mannequin.glb\", \"paint\": { \"M_Main\": \"team\" } } ] }\n" ) );
          const FigurePack pack = FigurePack::read( dir, false );
          QVERIFY2( pack.ok(), qPrintable( pack.error ) );

          Figures3D mannequin;
          QVERIFY( mannequin.load() );
          const auto same = [&mannequin]( const Figures3D & figures ) {
              if ( figures.gaitClips().size() != mannequin.gaitClips().size() ) return QString( "clips" );
              for ( std::size_t k = 0; k < figures.gaitClips().size(); ++k )
              {
                  const GaitClip & got = figures.gaitClips()[k];
                  const GaitClip & want = mannequin.gaitClips()[k];
                  if ( std::fabs( got.speed / want.speed - 1.0f ) > 0.02f || std::fabs( got.duration - want.duration ) > 0.04f )
                  {
                      return QString( "clip %1: %2 m/s %3 s, not %4 m/s %5 s" ).arg( k ).arg( got.speed ).arg( got.duration ).arg( want.speed ).arg( want.duration );
                  }
              }
              const auto limbOf = []( const Figures3D & f, const FigureInstance & fi, const double time,
                                      const HumanBone upper, const HumanBone lower ) {
                  std::vector< QMatrix4x4 > world;
                  f.worldNodes( fi, time, world );
                  const HumanoidRig & rig = f.rig( f.bodyOf( fi ) );
                  return ( world[rig.node( lower )].column( 3 ) - world[rig.node( upper )].column( 3 ) ).toVector3D().normalized();
              };
              FigureInstance fi{ 0, 0.0f, 0.0f, 0.0f, 1.85f, QVector4D( 1.0f, 0.0f, 0.0f, 1.0f ) };
              for ( const float speed : { 1.0f, 3.0f, 6.0f, 8.6f } ) // walking, between walking and jogging, between jogging and sprinting, sprinting
              {
                  fi.motion.vx = speed;
                  for ( const double time : { 0.0, 0.3, 0.9 } )
                  {
                      fi.motion.gait = time * 0.75;
                      for ( const auto & limb : { std::make_pair( HumanBone::LeftUpperLeg, HumanBone::LeftLowerLeg ),
                                                  std::make_pair( HumanBone::RightLowerLeg, HumanBone::RightFoot ),
                                                  std::make_pair( HumanBone::LeftUpperArm, HumanBone::LeftLowerArm ) } )
                      {
                          const float dot = QVector3D::dotProduct( limbOf( figures, fi, time, limb.first, limb.second ),
                                                                   limbOf( mannequin, fi, time, limb.first, limb.second ) );
                          if ( dot < 0.995f )
                          {
                              return QString( "%1 m/s %2 s %3: %4" ).arg( speed ).arg( time ).arg( humanBoneName( limb.first ) ).arg( dot );
                          }
                      }
                      // the feet where the Mannequin's are: on the ground, under the figure
                      for ( const HumanBone foot : { HumanBone::LeftFoot, HumanBone::RightFoot } )
                      {
                          const auto at = [&]( const Figures3D & f ) {
                              std::vector< QMatrix4x4 > world;
                              f.worldNodes( fi, time, world );
                              return world[f.rig( f.bodyOf( fi ) ).node( foot )].column( 3 ).toVector3D();
                          };
                          const float off = ( at( figures ) - at( mannequin ) ).length();
                          if ( off > 0.06f )
                          {
                              return QString( "%1 m/s %2 s %3: %4 m off" ).arg( speed ).arg( time ).arg( humanBoneName( foot ) ).arg( off );
                          }
                      }
                  }
              }
              return QString();
          };
          Figures3D figures;
          QVERIFY( figures.load() );
          QVERIFY2( figures.setPack( pack ), qPrintable( figures.error() ) );
          QCOMPARE( same( figures ), QString() );

          // a BVH file as the motion's model: its own clip (named after the file), the body another model
          QVERIFY( writeFile( dir + "/figure.json",
                              "{ \"format\": 1, \"motion\": { \"model\": \"WalkUnder.bvh\", \"clips\": {\n"
                              "  \"idle\": { \"file\": \"Mannequin.glb\", \"clip\": \"Idle_Loop\" }, \"walk\": \"WalkUnder\",\n"
                              "  \"jog\": \"JogDown.bvh\", \"sprint\": \"SprintSide.bvh\" } },\n"
                              "  \"bodies\": [ { \"model\": \"Mannequin.glb\", \"paint\": { \"M_Main\": \"team\" } } ] }\n" ) );
          const FigurePack bare = FigurePack::read( dir, false );
          QVERIFY2( bare.ok(), qPrintable( bare.error ) );
          Figures3D skeleton;
          QVERIFY( skeleton.load() );
          QVERIFY2( skeleton.setPack( bare ), qPrintable( skeleton.error() ) );
          QVERIFY( skeleton.motionModel().vertices.empty() );
          QCOMPARE( skeleton.motionRig().upAxis(), QVector3D( 0.0f, 1.0f, 0.0f ) ); // stood upright
          QVERIFY( std::fabs( skeleton.motionRig().facingAngle() ) < 2.0f ); // and turned to face +z
          QCOMPARE( same( skeleton ), QString() );

          // the walk as Blender's BVH exporter writes it (test/data/bvh: z up, in centimeters, Y X Z,
          // a root "root" above the hips, positions on every joint)
          QVERIFY( QFile::copy( QStringLiteral( TEST_SOURCE_DIR "/test/data/bvh/walk.bvh" ), dir + "/BlenderWalk.bvh" ) );
          QVERIFY( writeFile( dir + "/figure.json",
                              "{ \"format\": 1, \"motion\": { \"model\": \"Mannequin.glb\", \"skeleton\": \"quaternius\", \"clips\": {\n"
                              "  \"idle\": \"Idle_Loop\", \"walk\": \"BlenderWalk.bvh\", \"jog\": \"Jog_Fwd_Loop\", \"sprint\": \"Sprint_Loop\" } },\n"
                              "  \"bodies\": [ { \"model\": \"Mannequin.glb\", \"paint\": { \"M_Main\": \"team\" } } ] }\n" ) );
          const FigurePack blender = FigurePack::read( dir, false );
          QVERIFY2( blender.ok(), qPrintable( blender.error ) );
          Figures3D blended;
          QVERIFY( blended.load() );
          QVERIFY2( blended.setPack( blender ), qPrintable( blended.error() ) );
          QCOMPARE( same( blended ), QString() );
      }

    /*!
      a pack of the user's moved to the trash: its folder with its files,
      no longer found; not a bundled one, nor a folder that is not a
      pack's own (without figure.json, the home folder, a user's data
      folder of packs), nor one gone: refused, nothing touched
    */
    void moveToTrash()
      {
          // HOME in the temporary folder too: the trash in the XDG data folder there
          const QByteArray real_home = qgetenv( "HOME" );
          const QString home = M_data.filePath( "home" );
          QDir().mkpath( home );
          qputenv( "HOME", QFile::encodeName( home ) );
          struct Back { QByteArray h; ~Back() { qputenv( "HOME", h ); } } back{ real_home };
          const QString user = FigurePacks::userFolders().value( 0 );
          const QString trash = QStandardPaths::writableLocation( QStandardPaths::GenericDataLocation ) + "/Trash";
          QCOMPARE( trash, home + "/Trash" );

          const QString dir = makePack( user, "gone", packJson(), { "Body.glb" } );
          FigurePacks::refresh();
          const FigurePack * pack = FigurePacks::find( "gone", QString() );
          QVERIFY( pack && pack->ok() );
          const FigurePack copy = *pack;
          QString error;
          QVERIFY2( FigurePacks::moveToTrash( copy, &error ), qPrintable( error ) );
          QVERIFY( ! QFileInfo::exists( dir ) );
          QVERIFY( QFileInfo( trash + "/files/gone/figure.json" ).isFile() );
          QVERIFY( QFileInfo( trash + "/files/gone/Body.glb" ).isFile() );
          QVERIFY( ! FigurePacks::find( "gone", QString() ) ); // looked for again
          // gone already
          QVERIFY( ! FigurePacks::moveToTrash( copy, &error ) );
          QCOMPARE( error, "no folder \"" + dir + "\"" );

          // a bundled one
          QVERIFY( ! FigurePacks::moveToTrash( *FigurePacks::builtin( "mannequin" ), &error ) );
          QCOMPARE( error, QString( "a bundled pack" ) );
          FigurePack mine;
          mine.id = "mine";
          // without figure.json: not a pack's own folder
          mine.dir = makePack( user, "plain", QByteArray(), { "Body.glb" } );
          QVERIFY( ! FigurePacks::moveToTrash( mine, &error ) );
          QCOMPARE( error, "no figure.json in \"" + mine.dir + "\"" );
          QVERIFY( QFileInfo( mine.dir + "/Body.glb" ).isFile() );
          // the home folder, a user's data folder of packs, each with a figure.json
          for ( const QString & d : { home, user } )
          {
              QVERIFY( writeFile( d + "/figure.json", packJson() ) );
              mine.dir = d;
              QVERIFY( ! FigurePacks::moveToTrash( mine, &error ) );
              QCOMPARE( error, "\"" + d + "\" is not a pack's own folder" );
              QVERIFY( QFileInfo( d + "/figure.json" ).isFile() );
              QFile::remove( d + "/figure.json" );
          }
          QDir( user + "/plain" ).removeRecursively();
          QDir( trash ).removeRecursively();
          FigurePacks::refresh();
      }

    /*!
      a pack of the user's drawn (Figures3D::setPack()): a body of its own
      file with the bundled motion moves as the Mannequin does; its own
      motion; the skeleton found from the bones' names; each read once
    */
    void setPack()
      {
          QTemporaryDir folder;
          const QString f = folder.path();
          makePack( f, "robot", packJson(), { "Body.glb" } );
          makePack( f, "own", packJson( QByteArray(), OWN_MOTION ), { "Body.glb", "Motion.glb" } );
          const std::vector< FigurePack > list = FigurePacks::scan( f );

          Figures3D mannequin;
          QVERIFY2( mannequin.load(), qPrintable( mannequin.error() ) );
          Figures3D figures;
          QVERIFY( figures.load() );
          QVERIFY2( figures.setPack( *byId( list, "robot" ) ), qPrintable( figures.error() ) );
          QCOMPARE( figures.pack().id, QString( "robot" ) );
          QCOMPARE( figures.bodyCount(), 1 );
          QCOMPARE( figures.jerseyMaterials(), QStringList( { "M_Main" } ) );
          // the same model in another file, with the bundled motion: the same poses
          SceneContent a, b;
          for ( SceneContent * c : { &a, &b } )
          {
              for ( int i = 0; i < 3; ++i )
              {
                  FigureInstance fi{ i, 3.0f * i, 1.0f, 30.0f * i, 1.85f, QVector4D( 1.0f, 0.0f, 0.0f, 1.0f ) };
                  fi.motion.gait = 0.3 * i;
                  fi.motion.vx = 1.5f * i;
                  c->figures.push_back( fi );
              }
          }
          mannequin.pose( a, 2.0 );
          figures.pose( b, 2.0 );
          QCOMPARE( a.figure_joints.size(), b.figure_joints.size() );
          QVERIFY( std::fabs( figures.bodyStride() - 1.0f ) < 1.0e-4f );
          for ( std::size_t i = 0; i < a.figure_joints.size(); ++i )
          {
              QVERIFY2( std::fabs( a.figure_joints[i] - b.figure_joints[i] ) < 1.0e-3f,
                        qPrintable( QString( "%1: %2 %3" ).arg( i ).arg( a.figure_joints[i] ).arg( b.figure_joints[i] ) ) );
          }

          // its own motion, the skeleton found (none named for the body)
          QVERIFY2( figures.setPack( *byId( list, "own" ) ), qPrintable( figures.error() ) );
          QCOMPARE( figures.pack().id, QString( "own" ) );
          QCOMPARE( int( figures.gaitClips().size() ), 3 );
          QVERIFY( figures.gaitClips()[0].speed < figures.gaitClips()[2].speed );

          // each read once
          const GltfModel * robot_body = nullptr;
          QVERIFY( figures.setPack( *byId( list, "robot" ) ) );
          robot_body = &figures.model();
          QVERIFY( figures.setPack( *byId( list, "own" ) ) );
          QVERIFY( figures.setPack( *byId( list, "robot" ) ) );
          QCOMPARE( &figures.model(), robot_body );
      }

    /*!
      packs of glTF models of other skeletons (humanoids of boxes,
      test_gltf.h) with the bundled motion: an A pose of Mixamo's names
      (with a prefix) and a texture, its arms moving as the Mannequin's; a
      VRM 0.x facing -z, by its own roles, turned to face +z; a .gltf with
      its files beside it, by a skeleton map; the paints of the shades
    */
    void gltfPacks()
      {
          QTemporaryDir folder;
          const QString f = folder.path();
          QImage texture( 4, 4, QImage::Format_RGBA8888 );
          texture.fill( QColor( 200, 150, 100 ) );

          TestGltf::HumanoidOptions a;
          a.preset = "mixamo";
          a.prefix = "mixamorig:";
          a.arm_down = 40.0f;
          a.texture = texture;
          QVERIFY( QDir( f ).mkpath( "apose" ) );
          QVERIFY( TestGltf::File::write( QDir( f ).filePath( "apose/Body.glb" ), TestGltf::humanoid( TestGltf::mannequin(), a ).glb() ) );
          QVERIFY( writeFile( QDir( f ).filePath( "apose/figure.json" ),
                              "{ \"format\": 1, \"motion\": \"mannequin\", \"bodies\": [ { \"model\": \"Body.glb\", \"skeleton\": \"mixamo\","
                              " \"paint\": { \"Body\": \"skin\" } } ] }" ) );

          TestGltf::HumanoidOptions v;
          v.preset = "vroid";
          v.facing = 180.0f;
          v.vrm = 0;
          QVERIFY( QDir( f ).mkpath( "vrm" ) );
          QVERIFY( TestGltf::File::write( QDir( f ).filePath( "vrm/Avatar.vrm" ), TestGltf::humanoid( TestGltf::mannequin(), v ).glb() ) );
          QVERIFY( writeFile( QDir( f ).filePath( "vrm/figure.json" ),
                              "{ \"format\": 1, \"motion\": \"mannequin\", \"bodies\": [ { \"model\": \"Avatar.vrm\","
                              " \"paint\": { \"Body\": \"tint\" } } ] }" ) );

          TestGltf::HumanoidOptions t;
          t.facing = -70.0f; // to the left of +z
          QVERIFY( QDir( f ).mkpath( "side" ) );
          QVERIFY( TestGltf::File::write( QDir( f ).filePath( "side/Body.glb" ), TestGltf::humanoid( TestGltf::mannequin(), t ).glb() ) );
          QVERIFY( writeFile( QDir( f ).filePath( "side/figure.json" ),
                              "{ \"format\": 1, \"motion\": \"mannequin\", \"bodies\": [ { \"model\": \"Body.glb\" } ] }" ) );

          TestGltf::HumanoidOptions g;
          g.preset = "quaternius";
          QVERIFY( QDir( f ).mkpath( "files" ) );
          TestGltf::File named = TestGltf::humanoid( TestGltf::mannequin(), g );
          for ( TestGltf::Node & n : named.nodes ) n.name = "B-" + n.name; // no preset's
          QVERIFY( named.writeGltf( QDir( f ).filePath( "files" ), "body" ) );
          QByteArray map = "{ ";
          for ( int b = 0; b < HUMAN_BONE_COUNT; ++b )
          {
              map += QByteArray( b ? ", " : "" ) + "\"" + humanBoneName( HumanBone( b ) ) + "\": \"B-" + quaterniusBoneNames()[b].toUtf8() + "\"";
          }
          map += " }";
          QVERIFY( writeFile( QDir( f ).filePath( "files/figure.json" ),
                              "{ \"format\": 1, \"motion\": \"mannequin\", \"bodies\": [ { \"model\": \"body.gltf\", \"skeleton\": " + map + " } ] }" ) );
          QVERIFY( QDir( f ).mkpath( "unknown" ) );
          QVERIFY( named.writeGltf( QDir( f ).filePath( "unknown" ), "body" ) );
          QVERIFY( writeFile( QDir( f ).filePath( "unknown/figure.json" ),
                              "{ \"format\": 1, \"motion\": \"mannequin\", \"bodies\": [ { \"model\": \"body.gltf\" } ] }" ) );

          const std::vector< FigurePack > list = FigurePacks::scan( f );
          Figures3D mannequin;
          QVERIFY( mannequin.load() );
          // a figure of each, at the same time and place, its arm's direction
          FigureInstance fi{ 0, 0.0f, 0.0f, 30.0f, 1.85f, QVector4D( 1.0f, 0.0f, 0.0f, 1.0f ) };
          const auto armOf = []( const Figures3D & figures, const FigureInstance & figure, const double time ) {
              std::vector< QMatrix4x4 > world;
              figures.worldNodes( figure, time, world );
              const HumanoidRig & rig = figures.rig( figures.bodyOf( figure ) );
              return ( world[rig.node( HumanBone::LeftLowerArm )].column( 3 )
                       - world[rig.node( HumanBone::LeftUpperArm )].column( 3 ) ).toVector3D().normalized();
          };
          const auto leftOf = []( const Figures3D & figures, const FigureInstance & figure ) {
              std::vector< QMatrix4x4 > world;
              figures.worldNodes( figure, 0.0, world );
              const HumanoidRig & rig = figures.rig( figures.bodyOf( figure ) );
              return ( world[rig.node( HumanBone::LeftUpperLeg )].column( 3 )
                       - world[rig.node( HumanBone::RightUpperLeg )].column( 3 ) ).toVector3D().normalized();
          };

          // the A pose: its arms as the Mannequin's
          {
              Figures3D figures;
              QVERIFY( figures.load() );
              QVERIFY2( figures.setPack( *byId( list, "apose" ) ), qPrintable( figures.error() ) );
              QVERIFY( ! figures.rig().restTurn( HumanBone::LeftUpperArm ).isIdentity() );
              QCOMPARE( figures.model().images.size(), std::size_t( 1 ) );
              QCOMPARE( figures.paints()[0].kind, int( FigureUniform::Paint::Skin ) );
              for ( const double time : { 0.0, 0.4, 1.1 } )
              {
                  const float dot = QVector3D::dotProduct( armOf( figures, fi, time ), armOf( mannequin, fi, time ) );
                  QVERIFY2( dot > std::cos( 3.0f * 3.14159265f / 180.0f ), qPrintable( QString( "%1: %2" ).arg( time ).arg( dot ) ) );
              }
          }
          // the VRM: by its roles, turned to face +z, its left on the Mannequin's left
          {
              Figures3D figures;
              QVERIFY( figures.load() );
              QVERIFY2( figures.setPack( *byId( list, "vrm" ) ), qPrintable( figures.error() ) );
              QCOMPARE( figures.model().vrm_version, 0 );
              QVERIFY( std::fabs( figures.rig().facingAngle() ) < 0.01f );
              QCOMPARE( figures.paints()[0].kind, int( FigureUniform::Paint::Tint ) );
              QVERIFY( QVector3D::dotProduct( leftOf( figures, fi ), leftOf( mannequin, fi ) ) > 0.999f );
              const float dot = QVector3D::dotProduct( armOf( figures, fi, 0.7 ), armOf( mannequin, fi, 0.7 ) );
              QVERIFY2( dot > 0.999f, qPrintable( QString::number( dot ) ) );
          }
          // a model facing another way: turned to face +z
          {
              Figures3D figures;
              QVERIFY( figures.load() );
              QVERIFY2( figures.setPack( *byId( list, "side" ) ), qPrintable( figures.error() ) );
              QVERIFY2( std::fabs( figures.rig().facingAngle() ) < 0.01f, qPrintable( QString::number( figures.rig().facingAngle() ) ) );
              QVERIFY( QVector3D::dotProduct( leftOf( figures, fi ), leftOf( mannequin, fi ) ) > 0.999f );
          }
          // the .gltf by the map; without it, of no skeleton known
          {
              Figures3D figures;
              QVERIFY( figures.load() );
              QVERIFY2( figures.setPack( *byId( list, "files" ) ), qPrintable( figures.error() ) );
              QCOMPARE( figures.model().node_names[figures.rig().node( HumanBone::Hips )], QString( "B-pelvis" ) );
              QVERIFY( ! figures.setPack( *byId( list, "unknown" ) ) );
              QVERIFY2( figures.error().startsWith( "unknown: body.gltf: the bones are of no skeleton known" ), qPrintable( figures.error() ) );
          }
      }

    /*!
      what only reading the models finds, told with the pack's and the
      file's names: not a model, a clip not in it (with its clips), a clip
      not of walking, a material not in it (with its materials), bones of
      no skeleton known; a pack that cannot be read; the pack drawn stays
    */
    /*!
      packs of clips of other ways: backward only, the gait going on
      backward and the hips turning going diagonally; one side only, the
      other side's mirrored, the forward clips played backward; a motion
      of the same model and clips but the optional ones read again; a
      clip that does not go its way told why
    */
    void wayPacks()
      {
          QTemporaryDir folder;
          const QString f = folder.path();
          const auto motionWith = []( const QByteArray & more ) {
              return "{ \"model\": \"Motion.glb\", \"skeleton\": \"quaternius\", \"clips\": { \"idle\": \"Idle_Loop\", "
                  "\"walk\": \"Walk_Loop\", \"jog\": \"Jog_Fwd_Loop\", \"sprint\": \"Sprint_Loop\", " + more + " } }";
          };
          const auto withClips = [&]( const char * name,
                                      const QByteArray & more ) {
              const QString dir = makePack( f, name, packJson( QByteArray(), motionWith( more ) ), { "Body.glb", "Motion.glb" } );
              for ( const char * bvh : { "WalkBack.bvh", "JogBack.bvh", "WalkRight.bvh", "JogLeft.bvh" } )
              {
                  QFile::copy( QStringLiteral( ":/figures/mannequin/" ) + bvh, QDir( dir ).filePath( bvh ) );
              }
          };
          withClips( "back", "\"walk_back\": \"WalkBack.bvh\"" );
          withClips( "left", "\"jog_left\": \"JogLeft.bvh\"" );
          withClips( "not_back", "\"walk_back\": \"Walk_Loop\"" );
          withClips( "not_side", "\"jog_right\": \"Walk_Loop\"" );
          withClips( "not_side_told", "\"jog_right\": \"Walk_Loop\", \"jog_left\": \"JogLeft.bvh\"" );
          const std::vector< FigurePack > list = FigurePacks::scan( f );
          Figures3D figures;
          QVERIFY( figures.load() );
          const auto names = [&]( const int way ) {
              QStringList n;
              for ( const GaitClip & g : figures.wayClips( way ) ) n << figures.motionModel().clips[g.clip].name + ( g.reverse ? " <" : "" );
              return n;
          };

          // backward only: forward and backward mixed about sideways, the hips turned
          QVERIFY2( figures.setPack( *byId( list, "back" ) ), qPrintable( figures.error() ) );
          // (the forward walk faster than the backward one)
          QCOMPARE( names( GaitWays::Backward ), QStringList( { "WalkBack.bvh", "Walk_Loop <", "Jog_Fwd_Loop <", "Sprint_Loop <" } ) );
          QVERIFY( names( GaitWays::Left ).isEmpty() && names( GaitWays::Right ).isEmpty() );
          QVERIFY( figures.gaitWays().directional() );
          QVERIFY( ! figures.gaitWays().sideways() );
          FigureInstance diagonal{ 0, 0.0f, 0.0f, 0.0f, 1.85f, QVector4D( 1.0f, 0.0f, 0.0f, 1.0f ) };
          diagonal.motion.vx = diagonal.motion.vy = 2.0f;
          QVERIFY( std::fabs( figures.twistOf( diagonal ) - Figures3D::TWIST_MAX ) < 1.0e-3f );
          diagonal.motion.vx = -2.0f;
          diagonal.motion.vy = 0.0f;
          QCOMPARE( figures.gaitOf( diagonal ).ways.b, int( GaitWays::Backward ) );
          QCOMPARE( figures.gaitOf( diagonal ).ways.t, 1.0f );

          // one side only: the other mirrored, backward the forward ones played backward
          QVERIFY2( figures.setPack( *byId( list, "left" ) ), qPrintable( figures.error() ) );
          QCOMPARE( names( GaitWays::Left ), QStringList( { "JogLeft.bvh" } ) );
          QCOMPARE( names( GaitWays::Right ), QStringList( { "JogLeft.bvh (mirrored)" } ) );
          QCOMPARE( names( GaitWays::Backward ), QStringList( { "Walk_Loop <", "Jog_Fwd_Loop <", "Sprint_Loop <" } ) );
          QVERIFY( figures.gaitWays().sideways() );
          diagonal.motion.vx = diagonal.motion.vy = 2.0f;
          QVERIFY( std::fabs( figures.twistOf( diagonal ) - 22.5f ) < 1.0e-3f ); // 45 degrees: halfway from 30 to sideways

          // the same motion but the optional clips: read again
          FigurePack plain = *byId( list, "left" );
          plain.id = QStringLiteral( "plain" );
          plain.motion.jog_left.given = false;
          QVERIFY2( figures.setPack( plain ), qPrintable( figures.error() ) );
          QVERIFY( figures.wayClips( GaitWays::Left ).empty() );
          QVERIFY( ! figures.gaitWays().directional() );

          // not going its way
          const auto fails = [&]( const char * id, const QString & why ) {
              QVERIFY( ! figures.setPack( *byId( list, id ) ) );
              QVERIFY2( figures.error().contains( why ), qPrintable( figures.error() ) );
          };
          fails( "not_back", "A Pack: Motion.glb: the clip \"Walk_Loop\" is not of walking or running backward "
                 "(the feet do not come down and go against the way)" );
          fails( "not_side", "A Pack: Motion.glb: the clip \"Walk_Loop\" mirrored is not of walking or running to the left" );
          fails( "not_side_told", "A Pack: Motion.glb: the clip \"Walk_Loop\" is not of walking or running to the right" );
          QCOMPARE( figures.pack().id, QString( "plain" ) ); // stays
      }

    void setPackErrors()
      {
          QTemporaryDir folder;
          const QString f = folder.path();
          const QByteArray no_clip = "{ \"model\": \"Motion.glb\", \"clips\": "
              "{ \"idle\": \"Idle_Loop\", \"walk\": \"Run_Loop\", \"jog\": \"Jog_Fwd_Loop\", \"sprint\": \"Sprint_Loop\" } }";
          const QByteArray idle_walk = "{ \"model\": \"Motion.glb\", \"clips\": "
              "{ \"idle\": \"Idle_Loop\", \"walk\": \"Idle_Loop\", \"jog\": \"Jog_Fwd_Loop\", \"sprint\": \"Sprint_Loop\" } }";
          makePack( f, "garbage", packJson(), {} );
          writeFile( QDir( f ).filePath( "garbage/Body.glb" ), "not a model" );
          makePack( f, "no_clip", packJson( QByteArray(), no_clip ), { "Body.glb", "Motion.glb" } );
          makePack( f, "idle_walk", packJson( QByteArray(), idle_walk ), { "Body.glb", "Motion.glb" } );
          makePack( f, "no_material", packJson( ", \"hairstyles\": [ \"Hair_X\" ]" ), { "Body.glb" } );
          makePack( f, "broken", "{ \"format\": 1 }" );
          const std::vector< FigurePack > list = FigurePacks::scan( f );

          Figures3D figures;
          QVERIFY( figures.load() );
          const auto fails = [&]( const char * id, const QString & why ) {
              QVERIFY( byId( list, id ) );
              QVERIFY( ! figures.setPack( *byId( list, id ) ) );
              QVERIFY2( figures.error().contains( why ), qPrintable( figures.error() ) );
              QCOMPARE( figures.pack().id, QString( "mannequin" ) ); // stays
          };
          fails( "garbage", "A Pack: Body.glb: " );
          fails( "no_clip", "A Pack: Motion.glb: no clip \"Run_Loop\" (its clips: " );
          fails( "no_clip", "\"Walk_Loop\"" );
          fails( "idle_walk", "A Pack: Motion.glb: the clip \"Idle_Loop\" is not of walking or running" );
          fails( "no_material", "A Pack: Body.glb: no material \"Hair_X\" (its materials: \"M_Main\", \"M_Joints\")" );
          fails( "broken", "broken: no \"motion\"" );
          QVERIFY( figures.setPack( *FigurePacks::builtin( "human" ) ) );
      }
};

QTEST_MAIN( TestFigurePack )
#include "test_figure_pack.moc"
