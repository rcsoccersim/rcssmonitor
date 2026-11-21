// -*-c++-*-

/*!
  \file bvh_model.cpp
  \brief a BVH file (motion capture) read into a GltfModel. Source File.
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

#include "bvh_model.h"

#include "gltf_model.h"

#include <QFileInfo>

#include <algorithm>
#include <cctype>
#include <set>
#include <vector>

namespace {

//! a node's channel in a frame of the motion
enum class Channel {
    XPosition, YPosition, ZPosition,
    XRotation, YRotation, ZRotation,
};

//! a ROOT or a JOINT: its node, and its channels (the values of a frame from first on)
struct Joint {
    int node = -1;
    std::vector< Channel > channels;
    int first = 0;
};

/*!
  \brief the words of a BVH file, one by one (split at white space)
*/
class Words {
public:
    explicit Words( const QByteArray & data )
        : M_p( data.constData() ),
          M_end( data.constData() + data.size() )
      { }

    //! the next word (empty at the end)
    QByteArray next()
      {
          while ( M_p < M_end && std::isspace( static_cast< unsigned char >( *M_p ) ) ) ++M_p;
          const char * start = M_p;
          while ( M_p < M_end && ! std::isspace( static_cast< unsigned char >( *M_p ) ) ) ++M_p;
          return QByteArray( start, static_cast< int >( M_p - start ) );
      }

    //! the next word as a number (the C locale, whatever the user's)
    bool number( float * value )
      {
          bool ok = false;
          *value = next().toFloat( &ok );
          return ok;
      }

private:
    const char * M_p;
    const char * M_end;
};

/*!
  \brief the hierarchy of a BVH file read into a model's nodes, and the
  joints' channels
*/
class Hierarchy {
public:
    Hierarchy( Words & words,
               GltfModel & model )
        : M_words( words ),
          M_model( model )
      { }

    std::vector< Joint > joints;
    int channel_count = 0; //!< the values of a frame
    QString error;

    //! a ROOT or a JOINT whose name is next, and all under it
    bool joint( const int parent )
      {
          const QByteArray name = M_words.next();
          if ( name.isEmpty() || name == "{" ) return fail( QStringLiteral( "a joint without a name" ) );
          Joint j;
          j.node = addNode( QString::fromUtf8( name ), parent );
          if ( M_words.next() != "{" ) return fail( QStringLiteral( "no { after the joint " ) + nameOf( j.node ) );
          const std::size_t index = joints.size();
          joints.push_back( j );
          while ( true )
          {
              const QByteArray w = M_words.next();
              if ( w == "}" ) return true;
              if ( w == "OFFSET" )
              {
                  if ( ! offset( j.node ) ) return false;
              }
              else if ( w == "CHANNELS" )
              {
                  float n = 0.0f;
                  if ( ! M_words.number( &n ) || n < 0.0f || n > 6.0f || n != static_cast< int >( n ) )
                  {
                      return fail( QStringLiteral( "a wrong number of channels at " ) + nameOf( j.node ) );
                  }
                  Joint & me = joints[index];
                  me.first = channel_count;
                  me.channels.clear();
                  for ( int c = 0; c < static_cast< int >( n ); ++c )
                  {
                      Channel ch;
                      if ( ! channelOf( M_words.next(), &ch ) )
                      {
                          return fail( QStringLiteral( "an unknown channel at " ) + nameOf( j.node ) );
                      }
                      me.channels.push_back( ch );
                  }
                  channel_count += static_cast< int >( n );
              }
              else if ( w == "JOINT" )
              {
                  if ( ! joint( j.node ) ) return false;
              }
              else if ( w == "End" )
              {
                  if ( M_words.next() != "Site" || M_words.next() != "{" )
                  {
                      return fail( QStringLiteral( "a broken End Site at " ) + nameOf( j.node ) );
                  }
                  const int end = addNode( nameOf( j.node ) + QStringLiteral( "_End" ), j.node );
                  if ( M_words.next() != "OFFSET" || ! offset( end ) || M_words.next() != "}" )
                  {
                      return fail( QStringLiteral( "a broken End Site at " ) + nameOf( j.node ) );
                  }
              }
              else if ( w.isEmpty() )
              {
                  return fail( QStringLiteral( "the hierarchy is cut short" ) );
              }
              else
              {
                  return fail( QStringLiteral( "\"" ) + QString::fromUtf8( w ) + QStringLiteral( "\" in the joint " ) + nameOf( j.node ) );
              }
          }
      }

private:
    Words & M_words;
    GltfModel & M_model;
    std::set< QString > M_names;

    bool fail( const QString & why )
      {
          if ( error.isEmpty() ) error = why;
          return false;
      }

    QString nameOf( const int node ) const { return M_model.node_names[node]; }

    //! a node (at its parent's place until its OFFSET), of a name not taken yet
    int addNode( QString name,
                 const int parent )
      {
          if ( M_names.count( name ) )
          {
              int k = 2;
              while ( M_names.count( name + QLatin1Char( '.' ) + QString::number( k ) ) ) ++k;
              name += QLatin1Char( '.' ) + QString::number( k );
          }
          M_names.insert( name );
          const int node = static_cast< int >( M_model.node_names.size() );
          M_model.node_names.push_back( name );
          M_model.parent.push_back( parent );
          M_model.rest.push_back( NodeTransform() );
          M_model.order.push_back( node ); // read parents first
          return node;
      }

    bool offset( const int node )
      {
          float x = 0.0f, y = 0.0f, z = 0.0f;
          if ( ! M_words.number( &x ) || ! M_words.number( &y ) || ! M_words.number( &z ) )
          {
              return fail( QStringLiteral( "a broken OFFSET at " ) + nameOf( node ) );
          }
          M_model.rest[node].t = QVector3D( x, y, z );
          return true;
      }

    static bool channelOf( const QByteArray & w,
                           Channel * ch )
      {
          const QByteArray l = w.toLower();
          if ( l == "xposition" ) *ch = Channel::XPosition;
          else if ( l == "yposition" ) *ch = Channel::YPosition;
          else if ( l == "zposition" ) *ch = Channel::ZPosition;
          else if ( l == "xrotation" ) *ch = Channel::XRotation;
          else if ( l == "yrotation" ) *ch = Channel::YRotation;
          else if ( l == "zrotation" ) *ch = Channel::ZRotation;
          else return false;
          return true;
      }
};

}

/*-------------------------------------------------------------------*/
bool
BvhModel::isBvh( const QByteArray & data,
                 const QString & path )
{
    int i = 0;
    if ( data.startsWith( "\xEF\xBB\xBF" ) ) i = 3; // a byte order mark
    while ( i < data.size() && std::isspace( static_cast< unsigned char >( data[i] ) ) ) ++i;
    return data.mid( i, 9 ) == "HIERARCHY"
        || path.endsWith( QLatin1String( ".bvh" ), Qt::CaseInsensitive );
}

/*-------------------------------------------------------------------*/
bool
BvhModel::read( const QByteArray & data,
                const QString & path,
                GltfModel & model,
                QString * error )
{
    const auto fail = [&]( const QString & why ) {
        model.clear();
        if ( error ) *error = why;
        return false;
    };

    model.clear();
    QByteArray text = data;
    if ( text.startsWith( "\xEF\xBB\xBF" ) ) text.remove( 0, 3 );
    Words words( text );
    if ( words.next() != "HIERARCHY" ) return fail( QStringLiteral( "not a BVH file (no HIERARCHY)" ) );

    // the skeleton: one root or more
    Hierarchy hierarchy( words, model );
    QByteArray w = words.next();
    if ( w != "ROOT" ) return fail( QStringLiteral( "no ROOT in the hierarchy" ) );
    while ( w == "ROOT" )
    {
        if ( ! hierarchy.joint( -1 ) ) return fail( hierarchy.error );
        w = words.next();
    }
    if ( w != "MOTION" ) return fail( QStringLiteral( "no MOTION after the hierarchy" ) );
    model.capture = true; // its root placed in the room it was taken in

    // the motion: Frames: n, Frame Time: s, then a line a frame
    float frames = 0.0f;
    float frame_time = 0.0f;
    if ( words.next() != "Frames:" || ! words.number( &frames ) || frames < 0.0f )
    {
        return fail( QStringLiteral( "no number of frames" ) );
    }
    if ( words.next() != "Frame" || words.next() != "Time:" || ! words.number( &frame_time ) )
    {
        return fail( QStringLiteral( "no frame time" ) );
    }
    if ( frames > 0.0f && frame_time <= 0.0f ) return fail( QStringLiteral( "a frame time not above 0" ) );

    // the frames whole (a file cut short keeps those it has)
    const int count = hierarchy.channel_count;
    std::vector< float > values;
    values.reserve( static_cast< std::size_t >( frames ) * count );
    int whole = 0;
    std::vector< float > row( count );
    while ( whole < static_cast< int >( frames ) )
    {
        bool ok = true;
        for ( int c = 0; c < count && ok; ++c ) ok = words.number( &row[c] );
        if ( ! ok ) break;
        values.insert( values.end(), row.begin(), row.end() );
        ++whole;
    }
    if ( whole == 0 ) return true; // a skeleton without a motion

    // the clip: a key a frame for each joint with channels
    GltfModel::Clip clip;
    clip.name = path.isEmpty() ? QStringLiteral( "motion" ) : QFileInfo( path ).completeBaseName();
    clip.duration = frame_time * ( whole - 1 );
    clip.nodes.resize( model.rest.size() );
    for ( const Joint & j : hierarchy.joints )
    {
        const bool turns = std::any_of( j.channels.begin(), j.channels.end(),
                                        []( const Channel c ) { return c >= Channel::XRotation; } );
        const bool moves = std::any_of( j.channels.begin(), j.channels.end(),
                                        []( const Channel c ) { return c <= Channel::ZPosition; } );
        if ( ! turns && ! moves ) continue;
        GltfModel::NodeChannels & nc = clip.nodes[j.node];
        for ( int f = 0; f < whole; ++f )
        {
            const float * v = values.data() + static_cast< std::size_t >( f ) * count + j.first;
            QQuaternion r;
            QVector3D t = model.rest[j.node].t; // a position channel takes the OFFSET's place
            for ( std::size_t c = 0; c < j.channels.size(); ++c )
            {
                switch ( j.channels[c] ) {
                case Channel::XPosition: t.setX( v[c] ); break;
                case Channel::YPosition: t.setY( v[c] ); break;
                case Channel::ZPosition: t.setZ( v[c] ); break;
                case Channel::XRotation: r = r * QQuaternion::fromAxisAndAngle( 1.0f, 0.0f, 0.0f, v[c] ); break;
                case Channel::YRotation: r = r * QQuaternion::fromAxisAndAngle( 0.0f, 1.0f, 0.0f, v[c] ); break;
                case Channel::ZRotation: r = r * QQuaternion::fromAxisAndAngle( 0.0f, 0.0f, 1.0f, v[c] ); break;
                }
            }
            const float time = frame_time * f;
            if ( turns )
            {
                r.normalize();
                nc.r.times.push_back( time );
                nc.r.values.insert( nc.r.values.end(), { r.x(), r.y(), r.z(), r.scalar() } );
            }
            if ( moves )
            {
                nc.t.times.push_back( time );
                nc.t.values.insert( nc.t.values.end(), { t.x(), t.y(), t.z() } );
            }
        }
    }
    model.clips.push_back( std::move( clip ) );
    return true;
}
