// -*-c++-*-

/*!
  \file figure_pack.cpp
  \brief the figure packs of the 3D view: the models and the motions of the human figures. Source File.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "figure_pack.h"

#include "figure_uniform.h"
#include "humanoid.h"

#include "camera_mode.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

#include <algorithm>
#include <map>

/*!
  \brief register the resources of the bundled packs, before they are read
  (outside a namespace: Q_INIT_RESOURCE declares the function of the
  resources in the namespace where it is used)
*/
static
void
initFigureResources()
{
    Q_INIT_RESOURCE( view3d_figures );
}

namespace {

//! the bundled packs, in this order
const char * BUILTIN_PACKS[] = { PLAYER_MODEL_MANNEQUIN, PLAYER_MODEL_HUMAN };
//! how many packs whose motion is another's are followed at most
constexpr int MOTION_DEPTH = 8;

//! a key or an index of figure.json as told: "bodies[1].paint"
QString
pathOf( const QString & parent,
        const QString & key )
{
    return parent.isEmpty() ? key : parent + QLatin1Char( '.' ) + key;
}

QString
quoted( const QString & s )
{
    return QLatin1Char( '"' ) + s + QLatin1Char( '"' );
}

/*!
  \class Reader
  \brief reads figure.json, keeping the first thing wrong with it
*/
class Reader {
public:
    explicit Reader( const QString & dir )
        : M_dir( dir )
      { }

    const QString & error() const { return M_error; }
    bool failed() const { return ! M_error.isEmpty(); }
    void fail( const QString & where,
               const QString & what )
      {
          if ( M_error.isEmpty() ) M_error = ( where.isEmpty() ? what : where + QStringLiteral( ": " ) + what );
      }

    //! a string of an object, if it is there (else def); a string if it is required
    QString string( const QJsonObject & o,
                    const QString & parent,
                    const char * key,
                    const bool required = false,
                    const QString & def = QString() )
      {
          const QJsonValue v = o.value( QLatin1String( key ) );
          if ( v.isUndefined() || v.isNull() )
          {
              if ( required ) fail( parent, QStringLiteral( "no " ) + quoted( QLatin1String( key ) ) );
              return def;
          }
          if ( ! v.isString() )
          {
              fail( pathOf( parent, QLatin1String( key ) ), QStringLiteral( "not a string" ) );
              return def;
          }
          return v.toString();
      }

    //! a list of strings of an object (none if it is not there), each once
    QStringList strings( const QJsonObject & o,
                         const QString & parent,
                         const char * key )
      {
          QStringList list;
          const QJsonValue v = o.value( QLatin1String( key ) );
          if ( v.isUndefined() || v.isNull() ) return list;
          const QString where = pathOf( parent, QLatin1String( key ) );
          if ( ! v.isArray() )
          {
              fail( where, QStringLiteral( "not a list" ) );
              return list;
          }
          const QJsonArray a = v.toArray();
          for ( int i = 0; i < a.size(); ++i )
          {
              if ( ! a[i].isString() || a[i].toString().isEmpty() )
              {
                  fail( where + QStringLiteral( "[%1]" ).arg( i ), QStringLiteral( "not a name" ) );
                  return list;
              }
              if ( list.contains( a[i].toString() ) )
              {
                  fail( where, quoted( a[i].toString() ) + QStringLiteral( " is listed twice" ) );
                  return list;
              }
              list << a[i].toString();
          }
          return list;
      }

    //! the path of a file of the pack named by a key, which must be there
    QString file( const QJsonObject & o,
                  const QString & parent,
                  const char * key )
      {
          const QString name = string( o, parent, key, true );
          if ( failed() ) return QString();
          if ( name.isEmpty() )
          {
              fail( pathOf( parent, QLatin1String( key ) ), QStringLiteral( "empty" ) );
              return QString();
          }
          const QString path = QDir::cleanPath( QDir( M_dir ).filePath( name ) );
          if ( ! QFileInfo( path ).isFile() )
          {
              fail( pathOf( parent, QLatin1String( key ) ), quoted( name ) + QStringLiteral( " is not found in the pack's folder" ) );
              return QString();
          }
          return path;
      }

    /*!
      a clip of the motion: the name of a clip of its model, the name of a
      file of the pack (by its extension: a model's file) for its first
      clip, or an object of a "file" and its "clip"
    */
    FigureClipSpec clip( const QJsonObject & o,
                         const QString & parent,
                         const char * key,
                         const bool optional = false )
      {
          FigureClipSpec c;
          const QJsonValue v = o.value( QLatin1String( key ) );
          const QString where = pathOf( parent, QLatin1String( key ) );
          if ( ( v.isUndefined() || v.isNull() ) && optional )
          {
              c.given = false;
          }
          else if ( v.isUndefined() || v.isNull() )
          {
              fail( parent, QStringLiteral( "no " ) + quoted( QLatin1String( key ) ) );
          }
          else if ( v.isString() )
          {
              if ( v.toString().isEmpty() ) fail( where, QStringLiteral( "empty" ) );
              else if ( isClipFile( v.toString() ) ) c.file = file( o, parent, key );
              else c.clip = v.toString();
          }
          else if ( v.isObject() )
          {
              c.file = file( v.toObject(), where, "file" );
              c.clip = string( v.toObject(), where, "clip" );
          }
          else
          {
              fail( where, QStringLiteral( "neither a clip's name, a file, nor an object of a \"file\" and a \"clip\"" ) );
          }
          return c;
      }

    //! a name of a file of clips (by its extension), not of a clip
    static bool isClipFile( const QString & name )
      {
          for ( const char * e : { ".fbx", ".glb", ".gltf", ".vrm", ".vrma", ".bvh" } )
          {
              if ( name.endsWith( QLatin1String( e ), Qt::CaseInsensitive ) ) return true;
          }
          return false;
      }

    /*!
      a skeleton: a preset, which must be known, or the pack's own map of
      the roles (as VRM names them) to the bones
    */
    QString skeleton( const QJsonObject & o,
                      const QString & parent,
                      std::vector< std::pair< QString, QString > > * bones )
      {
          const QString where = pathOf( parent, QStringLiteral( "skeleton" ) );
          const QJsonValue v = o.value( QStringLiteral( "skeleton" ) );
          if ( v.isObject() )
          {
              const QJsonObject map = v.toObject();
              for ( auto it = map.begin(); it != map.end(); ++it )
              {
                  if ( humanBoneOf( it.key() ) < 0 )
                  {
                      QStringList roles;
                      for ( int b = 0; b < HUMAN_BONE_COUNT; ++b ) roles << QString::fromLatin1( humanBoneName( HumanBone( b ) ) );
                      fail( where, quoted( it.key() ) + QStringLiteral( " is not one of " ) + roles.join( QStringLiteral( ", " ) ) );
                      return QString();
                  }
                  if ( ! it.value().isString() || it.value().toString().isEmpty() )
                  {
                      fail( pathOf( where, it.key() ), QStringLiteral( "not a bone's name" ) );
                      return QString();
                  }
                  bones->emplace_back( it.key(), it.value().toString() );
              }
              if ( bones->empty() ) fail( where, QStringLiteral( "empty" ) );
              return QString();
          }
          const QString preset = string( o, parent, "skeleton" );
          if ( ! preset.isEmpty() && ! skeletonPresets().contains( preset ) )
          {
              fail( where, quoted( preset ) + QStringLiteral( " is not one of " ) + skeletonPresets().join( QStringLiteral( ", " ) )
                    + QStringLiteral( ", nor a map of the roles to the bones" ) );
          }
          return preset;
      }

private:
    QString M_dir;
    QString M_error;
};

//! where the packs were looked for, and what was found
struct Cache {
    bool scanned = false;
    QString folder;
    std::vector< FigurePack > packs;
    int generation = 0;
    std::map< QString, QString > errors; //!< why a pack could not be drawn
};

Cache &
cache()
{
    static Cache c;
    return c;
}

//! the packs in a folder: the folder itself if it is a pack, else its folders
void
addFolder( const QString & folder,
           std::vector< FigurePack > & list )
{
    const QDir dir( folder );
    if ( folder.isEmpty() || ! dir.exists() ) return;

    QStringList dirs;
    if ( QFileInfo( dir.filePath( QLatin1String( FigurePacks::FILE_NAME ) ) ).isFile() )
    {
        dirs << dir.absolutePath();
    }
    else
    {
        for ( const QString & sub : dir.entryList( QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name ) )
        {
            dirs << dir.absoluteFilePath( sub );
        }
    }

    for ( const QString & d : dirs )
    {
        const QString id = QDir( d ).dirName();
        const auto same = std::find_if( list.begin(), list.end(),
                                        [&id]( const FigurePack & p ) { return p.id == id; } );
        if ( same != list.end() )
        {
            FigurePack p;
            p.id = id;
            p.dir = d;
            p.error = QStringLiteral( "another pack has the same name: " ) + same->dir;
            list.push_back( p );
        }
        else
        {
            list.push_back( FigurePack::read( d, false ) );
        }
    }
}

/*!
  \brief look up the motions of other packs: the model, the skeleton and
  the clips of the pack whose motion it is (through the packs whose motion
  is another's), and its credits
*/
void
resolveMotions( std::vector< FigurePack > & list )
{
    // looked up in the packs as read, so that the order does not matter
    const std::vector< FigurePack > read = list;
    const auto byId = [&read]( const QString & id ) -> const FigurePack * {
        for ( const FigurePack & p : read )
        {
            if ( p.id == id ) return &p; // the first of the name
        }
        return nullptr;
    };

    for ( FigurePack & p : list )
    {
        if ( ! p.ok() || p.motion.pack == p.id ) continue;
        QString last = p.motion.pack;
        const FigurePack * from = byId( last );
        int depth = 0;
        while ( from
                && from->ok()
                && from->motion.pack != from->id
                && from->id != p.id
                && depth < MOTION_DEPTH )
        {
            last = from->motion.pack;
            from = byId( last );
            ++depth;
        }
        if ( ! from )
        {
            p.error = QStringLiteral( "motion: no pack " ) + quoted( last );
            if ( depth > 0 ) p.error += QStringLiteral( " (the motion of " ) + quoted( p.motion.pack ) + QLatin1Char( ')' );
        }
        else if ( from->id == p.id || depth >= MOTION_DEPTH )
        {
            p.error = QStringLiteral( "motion: the packs' motions refer to each other" );
        }
        else if ( ! from->ok() )
        {
            p.error = QStringLiteral( "motion: the pack " ) + quoted( from->id ) + QStringLiteral( " cannot be read: " ) + from->error;
        }
        else
        {
            p.motion = from->motion;
            p.motion.credits = from->credits;
        }
    }
}

}

/*-------------------------------------------------------------------*/
QString
FigureCredit::text() const
{
    QString s = title;
    if ( ! author.isEmpty() )
    {
        s += ( s.isEmpty() ? QString() : QStringLiteral( " by " ) ) + author;
    }
    if ( ! license.isEmpty() )
    {
        s += ( s.isEmpty() ? license : QStringLiteral( " (" ) + license + QLatin1Char( ')' ) );
    }
    return s;
}

/*-------------------------------------------------------------------*/
QString
FigureClipSpec::text() const
{
    const QString name = QFileInfo( file ).fileName();
    if ( file.isEmpty() ) return quoted( clip );
    if ( clip.isEmpty() ) return name;
    return name + QLatin1Char( ':' ) + quoted( clip );
}

/*-------------------------------------------------------------------*/
const std::vector< const char * > &
FigureMotionSpec::optionalKeys()
{
    static const std::vector< const char * > keys = {
        "walk_back", "jog_back", "walk_left", "walk_right", "jog_left", "jog_right",
    };
    return keys;
}

/*-------------------------------------------------------------------*/
std::vector< const FigureClipSpec * >
FigureMotionSpec::optionalClips() const
{
    return { &walk_back, &jog_back, &walk_left, &walk_right, &jog_left, &jog_right };
}

/*-------------------------------------------------------------------*/
std::uint32_t
FigureBodySpec::bitOf( const QString & material ) const
{
    const int h = hairstyles.indexOf( material );
    if ( h >= 0 ) return 1u << h;
    for ( std::size_t e = 0; e < extras.size(); ++e )
    {
        if ( extras[e].material == material ) return 1u << ( hairstyles.size() + int( e ) );
    }
    return 0;
}

/*-------------------------------------------------------------------*/
bool
FigurePack::hasSexes() const
{
    const auto has = [this]( const FigureSex sex ) {
        return std::any_of( bodies.begin(), bodies.end(),
                            [sex]( const FigureBodySpec & b ) { return b.sex == sex; } );
    };
    return has( FigureSex::Male ) && has( FigureSex::Female );
}

/*-------------------------------------------------------------------*/
std::vector< FigureCredit >
FigurePack::allCredits() const
{
    std::vector< FigureCredit > all;
    for ( const std::vector< FigureCredit > * list : { &credits, &motion.credits } )
    {
        for ( const FigureCredit & c : *list )
        {
            if ( std::find( all.begin(), all.end(), c ) == all.end() ) all.push_back( c );
        }
    }
    return all;
}

/*-------------------------------------------------------------------*/
QString
FigurePack::creditLine() const
{
    QStringList parts;
    for ( const FigureCredit & c : allCredits() )
    {
        QString s = ( c.author.isEmpty() ? c.title : c.author );
        if ( ! c.license.isEmpty() )
        {
            s += ( s.isEmpty() ? c.license : QStringLiteral( " (" ) + c.license + QLatin1Char( ')' ) );
        }
        if ( ! s.isEmpty() && ! parts.contains( s ) ) parts << s;
    }
    return parts.join( QStringLiteral( "; " ) );
}

/*-------------------------------------------------------------------*/
QString
FigurePack::creditText() const
{
    QStringList lines;
    for ( const FigureCredit & c : allCredits() )
    {
        QString s = c.text();
        if ( ! c.url.isEmpty() ) s += ( s.isEmpty() ? c.url : QStringLiteral( "\n  " ) + c.url );
        if ( ! s.isEmpty() ) lines << s;
    }
    return lines.join( QLatin1Char( '\n' ) );
}

/*-------------------------------------------------------------------*/
FigureCast
FigurePack::cast() const
{
    FigureCast cast;
    for ( const FigureBodySpec & spec : bodies )
    {
        FigureCast::Body b;
        b.sex = spec.sex;
        for ( const QString & h : spec.hairstyles ) b.hairstyles.push_back( spec.bitOf( h ) );
        b.standard_hair = ( spec.hairstyles.isEmpty() ? 0 : spec.bitOf( spec.hairstyles[spec.standard_hairstyle] ) );
        for ( const FigureBodySpec::Extra & e : spec.extras )
        {
            b.extras.push_back( FigureCast::Extra{ spec.bitOf( e.material ), e.chance } );
        }
        cast.bodies.push_back( b );
    }
    return cast;
}

/*-------------------------------------------------------------------*/
FigurePack
FigurePack::read( const QString & dir,
                  const bool builtin )
{
    if ( builtin ) initFigureResources();

    FigurePack pack;
    pack.dir = dir;
    pack.id = QDir( dir ).dirName();
    pack.builtin = builtin;

    QFile file( QDir( dir ).filePath( QLatin1String( FigurePacks::FILE_NAME ) ) );
    if ( ! file.open( QIODevice::ReadOnly ) )
    {
        pack.error = QStringLiteral( "no " ) + QLatin1String( FigurePacks::FILE_NAME );
        return pack;
    }
    const QByteArray data = file.readAll();
    QJsonParseError parse;
    const QJsonDocument doc = QJsonDocument::fromJson( data, &parse );
    if ( parse.error != QJsonParseError::NoError )
    {
        const QByteArray before = data.left( parse.offset );
        const int line = before.count( '\n' ) + 1;
        const int column = int( parse.offset - ( before.lastIndexOf( '\n' ) + 1 ) ) + 1;
        pack.error = QStringLiteral( "%1: %2 (line %3, column %4)" )
            .arg( QLatin1String( FigurePacks::FILE_NAME ), parse.errorString() ).arg( line ).arg( column );
        return pack;
    }
    if ( ! doc.isObject() )
    {
        pack.error = QLatin1String( FigurePacks::FILE_NAME ) + QStringLiteral( ": not an object" );
        return pack;
    }
    const QJsonObject root = doc.object();
    Reader r( dir );

    // the format first: a later one may be written otherwise
    const QJsonValue format = root.value( QStringLiteral( "format" ) );
    if ( ! format.isDouble() || format.toDouble() != double( format.toInt() ) )
    {
        r.fail( QString(), QStringLiteral( "no \"format\" (%1)" ).arg( FigurePacks::FORMAT ) );
    }
    else if ( format.toInt() < 1 || format.toInt() > FigurePacks::FORMAT )
    {
        r.fail( QStringLiteral( "format" ),
                QStringLiteral( "%1, but this rcssmonitor reads %2" ).arg( format.toInt() ).arg( FigurePacks::FORMAT ) );
    }
    pack.format = format.toInt();
    pack.name = r.string( root, QString(), "name" );
    pack.description = r.string( root, QString(), "description" );

    // the credits: each an object, or a line
    const QJsonValue credits = root.value( QStringLiteral( "credits" ) );
    if ( ! credits.isUndefined() && ! credits.isNull() )
    {
        if ( ! credits.isArray() )
        {
            r.fail( QStringLiteral( "credits" ), QStringLiteral( "not a list" ) );
        }
        const QJsonArray a = credits.toArray();
        for ( int i = 0; i < a.size() && ! r.failed(); ++i )
        {
            const QString where = QStringLiteral( "credits[%1]" ).arg( i );
            FigureCredit c;
            if ( a[i].isString() )
            {
                c.title = a[i].toString();
            }
            else if ( a[i].isObject() )
            {
                const QJsonObject o = a[i].toObject();
                c.title = r.string( o, where, "title" );
                c.author = r.string( o, where, "author" );
                c.license = r.string( o, where, "license" );
                c.url = r.string( o, where, "url" );
            }
            else
            {
                r.fail( where, QStringLiteral( "neither an object nor a string" ) );
            }
            pack.credits.push_back( c );
        }
    }

    // the motion: its own, or another pack's by its name
    const QJsonValue motion = root.value( QStringLiteral( "motion" ) );
    if ( motion.isString() )
    {
        pack.motion.pack = motion.toString();
        if ( pack.motion.pack.isEmpty() ) r.fail( QStringLiteral( "motion" ), QStringLiteral( "empty" ) );
        else if ( pack.motion.pack == pack.id ) r.fail( QStringLiteral( "motion" ), QStringLiteral( "the pack itself" ) );
    }
    else if ( motion.isObject() )
    {
        const QJsonObject o = motion.toObject();
        const QString where = QStringLiteral( "motion" );
        pack.motion.pack = pack.id;
        pack.motion.model = r.file( o, where, "model" );
        pack.motion.skeleton = r.skeleton( o, where, &pack.motion.bones );
        const QJsonValue clips = o.value( QStringLiteral( "clips" ) );
        if ( ! clips.isObject() )
        {
            r.fail( where, QStringLiteral( "no \"clips\"" ) );
        }
        const QJsonObject c = clips.toObject();
        const QString at = pathOf( where, QStringLiteral( "clips" ) );
        pack.motion.idle = r.clip( c, at, "idle" );
        pack.motion.walk = r.clip( c, at, "walk" );
        pack.motion.jog = r.clip( c, at, "jog" );
        pack.motion.sprint = r.clip( c, at, "sprint" );
        pack.motion.walk_back = r.clip( c, at, "walk_back", true );
        pack.motion.jog_back = r.clip( c, at, "jog_back", true );
        pack.motion.walk_left = r.clip( c, at, "walk_left", true );
        pack.motion.walk_right = r.clip( c, at, "walk_right", true );
        pack.motion.jog_left = r.clip( c, at, "jog_left", true );
        pack.motion.jog_right = r.clip( c, at, "jog_right", true );
    }
    else
    {
        r.fail( QString(), QStringLiteral( "no \"motion\" (an object, or the name of a pack)" ) );
    }

    // the bodies
    const QJsonValue bodies = root.value( QStringLiteral( "bodies" ) );
    if ( ! bodies.isArray() || bodies.toArray().isEmpty() )
    {
        r.fail( QString(), QStringLiteral( "no \"bodies\" (a list of one or more)" ) );
    }
    const QJsonArray list = bodies.toArray();
    for ( int i = 0; i < list.size() && ! r.failed(); ++i )
    {
        const QString where = QStringLiteral( "bodies[%1]" ).arg( i );
        if ( ! list[i].isObject() )
        {
            r.fail( where, QStringLiteral( "not an object" ) );
            break;
        }
        const QJsonObject o = list[i].toObject();
        FigureBodySpec b;
        b.model = r.file( o, where, "model" );
        b.skeleton = r.skeleton( o, where, &b.bones );

        const QString sex = r.string( o, where, "sex" );
        if ( sex == QLatin1String( "male" ) ) b.sex = FigureSex::Male;
        else if ( sex == QLatin1String( "female" ) ) b.sex = FigureSex::Female;
        else if ( ! sex.isEmpty() ) r.fail( pathOf( where, QStringLiteral( "sex" ) ), quoted( sex ) + QStringLiteral( " is not male or female" ) );

        b.hairstyles = r.strings( o, where, "hairstyles" );
        const QString standard = r.string( o, where, "standard_hairstyle" );
        if ( ! standard.isEmpty() )
        {
            b.standard_hairstyle = b.hairstyles.indexOf( standard );
            if ( b.standard_hairstyle < 0 )
            {
                r.fail( pathOf( where, QStringLiteral( "standard_hairstyle" ) ),
                        quoted( standard ) + QStringLiteral( " is not one of the hairstyles" ) );
                b.standard_hairstyle = 0;
            }
        }

        const QJsonValue extras = o.value( QStringLiteral( "extras" ) );
        if ( ! extras.isUndefined() && ! extras.isNull() && ! extras.isArray() )
        {
            r.fail( pathOf( where, QStringLiteral( "extras" ) ), QStringLiteral( "not a list" ) );
        }
        const QJsonArray ex = extras.toArray();
        for ( int k = 0; k < ex.size() && ! r.failed(); ++k )
        {
            const QString at = pathOf( where, QStringLiteral( "extras[%1]" ).arg( k ) );
            const QJsonObject e = ex[k].toObject();
            FigureBodySpec::Extra extra;
            extra.material = r.string( e, at, "material", true );
            const QJsonValue chance = e.value( QStringLiteral( "chance" ) );
            extra.chance = chance.toDouble( -1.0 );
            if ( ! r.failed() && ( ! chance.isDouble() || extra.chance < 0.0 || extra.chance > 1.0 ) )
            {
                r.fail( at, QStringLiteral( "no \"chance\" from 0 to 1" ) );
            }
            if ( ! r.failed()
                 && ( b.hairstyles.contains( extra.material ) || b.bitOf( extra.material ) != 0 ) )
            {
                r.fail( at, quoted( extra.material ) + QStringLiteral( " is listed twice" ) );
            }
            b.extras.push_back( extra );
        }
        if ( ! r.failed()
             && b.hairstyles.size() + int( b.extras.size() ) > FigureUniform::SCALP_HAIRS )
        {
            r.fail( where, QStringLiteral( "more than %1 hairstyles and extras" ).arg( FigureUniform::SCALP_HAIRS ) );
        }

        // how the materials are painted: the hairstyles and the extras in the hair's color
        const QJsonValue paint = o.value( QStringLiteral( "paint" ) );
        if ( ! paint.isUndefined() && ! paint.isNull() && ! paint.isObject() )
        {
            r.fail( pathOf( where, QStringLiteral( "paint" ) ), QStringLiteral( "not an object" ) );
        }
        const QJsonObject p = paint.toObject();
        for ( auto it = p.begin(); it != p.end() && ! r.failed(); ++it )
        {
            const QString at = pathOf( where, QStringLiteral( "paint" ) );
            const QString how = it.value().toString();
            if ( b.bitOf( it.key() ) != 0 && how != QLatin1String( "hair" ) )
            {
                r.fail( at, quoted( it.key() ) + QStringLiteral( " is a hairstyle or an extra, in the hair's color" ) );
            }
            else if ( how == QLatin1String( "team" ) ) b.team << it.key();
            else if ( how == QLatin1String( "uniform" ) ) b.uniform << it.key();
            else if ( how == QLatin1String( "hair" ) ) { if ( b.bitOf( it.key() ) == 0 ) b.hair << it.key(); }
            else if ( how == QLatin1String( "skin" ) ) b.skin << it.key();
            else if ( how == QLatin1String( "tint" ) ) b.tint << it.key();
            else if ( how != QLatin1String( "model" ) )
            {
                r.fail( at, quoted( it.key() ) + QStringLiteral( ": " ) + quoted( how )
                        + QStringLiteral( " is not one of team, tint, uniform, skin, hair, model" ) );
            }
        }
        pack.bodies.push_back( b );
    }

    pack.error = r.error();
    return pack;
}

namespace FigurePacks {

/*-------------------------------------------------------------------*/
QString
builtinFolder()
{
    return QStringLiteral( ":/figures" );
}

/*-------------------------------------------------------------------*/
QStringList
userFolders()
{
    QStringList list;
    const QString own = QStandardPaths::writableLocation( QStandardPaths::GenericDataLocation );
    if ( ! own.isEmpty() ) list << QDir( own ).filePath( QStringLiteral( "rcssmonitor/figures" ) );
    for ( const QString & d : QStandardPaths::standardLocations( QStandardPaths::GenericDataLocation ) )
    {
        list << QDir( d ).filePath( QStringLiteral( "rcssmonitor/figures" ) );
    }
    list.removeDuplicates();
    return list;
}

/*-------------------------------------------------------------------*/
const std::vector< FigurePack > &
builtins()
{
    static const std::vector< FigurePack > list = []() {
        std::vector< FigurePack > l;
        for ( const char * id : BUILTIN_PACKS )
        {
            l.push_back( FigurePack::read( builtinFolder() + QLatin1Char( '/' ) + QLatin1String( id ), true ) );
        }
        resolveMotions( l );
        return l;
    }();
    return list;
}

/*-------------------------------------------------------------------*/
const FigurePack *
builtin( const QString & id )
{
    for ( const FigurePack & p : builtins() )
    {
        if ( p.id == id ) return &p;
    }
    return nullptr;
}

/*-------------------------------------------------------------------*/
std::vector< FigurePack >
scan( const QString & folder )
{
    std::vector< FigurePack > list;
    for ( const char * id : BUILTIN_PACKS )
    {
        list.push_back( FigurePack::read( builtinFolder() + QLatin1Char( '/' ) + QLatin1String( id ), true ) );
    }
    if ( ! folder.isEmpty() ) addFolder( folder, list );
    for ( const QString & d : userFolders() )
    {
        // the folder of the user's choice may be one of them
        if ( ! folder.isEmpty() && QFileInfo( d ) == QFileInfo( folder ) ) continue;
        addFolder( d, list );
    }
    // the name of the cylinders is not a pack's
    for ( FigurePack & p : list )
    {
        if ( p.ok() && p.id == QLatin1String( PLAYER_MODEL_SIMPLE ) )
        {
            p.error = quoted( p.id ) + QStringLiteral( " is the name of the cylinders" );
        }
    }
    resolveMotions( list );
    return list;
}

/*-------------------------------------------------------------------*/
const std::vector< FigurePack > &
packs( const QString & folder )
{
    Cache & c = cache();
    if ( ! c.scanned || folder != c.folder )
    {
        c.packs = scan( folder );
        c.folder = folder;
        c.scanned = true;
        ++c.generation;
    }
    return c.packs;
}

/*-------------------------------------------------------------------*/
const FigurePack *
find( const QString & id,
      const QString & folder )
{
    for ( const FigurePack & p : packs( folder ) )
    {
        if ( p.id == id ) return &p;
    }
    return nullptr;
}

/*-------------------------------------------------------------------*/
void
refresh()
{
    cache().scanned = false;
}

/*-------------------------------------------------------------------*/
bool
moveToTrash( const FigurePack & pack,
             QString * error )
{
    const auto fail = [error]( const QString & why ) {
        if ( error ) *error = why;
        return false;
    };
    if ( pack.builtin || pack.dir.isEmpty() || pack.dir.startsWith( QLatin1Char( ':' ) ) )
    {
        return fail( QStringLiteral( "a bundled pack" ) );
    }
    const QFileInfo info( pack.dir );
    if ( ! info.isDir() )
    {
        return fail( QStringLiteral( "no folder " ) + quoted( pack.dir ) );
    }
    // only a pack's own folder: never one holding other things too
    if ( ! QFileInfo( QDir( pack.dir ).filePath( QLatin1String( FILE_NAME ) ) ).isFile() )
    {
        return fail( QStringLiteral( "no " ) + QLatin1String( FILE_NAME ) + QStringLiteral( " in " ) + quoted( pack.dir ) );
    }
    const QString path = info.canonicalFilePath();
    bool own = ! QDir( path ).isRoot()
        && path != QFileInfo( QDir::homePath() ).canonicalFilePath();
    for ( const QString & d : userFolders() )
    {
        if ( QFileInfo( d ).exists() && QFileInfo( d ).canonicalFilePath() == path ) own = false;
    }
    if ( ! own )
    {
        return fail( quoted( pack.dir ) + QStringLiteral( " is not a pack's own folder" ) );
    }
    if ( ! QFile::moveToTrash( path ) )
    {
        return fail( QStringLiteral( "the trash does not take " ) + quoted( pack.dir ) );
    }
    refresh();
    return true;
}

/*-------------------------------------------------------------------*/
int
generation()
{
    return cache().generation;
}

/*-------------------------------------------------------------------*/
void
noteError( const QString & id,
           const QString & why )
{
    cache().errors[id] = why;
}

/*-------------------------------------------------------------------*/
QString
drawError( const QString & id )
{
    const auto it = cache().errors.find( id );
    return ( it == cache().errors.end() ? QString() : it->second );
}

}
