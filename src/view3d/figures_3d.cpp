// -*-c++-*-

/*!
  \file figures_3d.cpp
  \brief the players of the 3D view as human figures. Source File.
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

#include "figures_3d.h"

#include "figure_actions.h"
#include "scene_builder.h"

#include <QFile>
#include <QFileInfo>

#include <algorithm>
#include <array>
#include <cmath>

namespace {

//! the points of a clip read by analyzeGait()
constexpr int GAIT_SAMPLES = 240;
//! a foot is down while it is this part of its highest above its lowest
constexpr float FOOT_DOWN = 0.15f;
//! a foot of a walk is lifted at least this part of the model's height (a shuffle backward: 1.5 to 2.5 %)
constexpr float FOOT_LIFT = 0.01f;
//! a foot is on the ground while it goes back at this part of its usual speed while low
constexpr double FOOT_BACK = 0.5;

float
clamp01( const float v )
{
    return std::min( 1.0f, std::max( 0.0f, v ) );
}

float
clampTo( const float v,
         const float limit )
{
    return std::min( limit, std::max( -limit, v ) );
}

//! a rotation about the model's vertical (+y) or its left (+x), degree
QQuaternion
aboutY( const float degree )
{
    return QQuaternion::fromAxisAndAngle( 0.0f, 1.0f, 0.0f, degree );
}

QQuaternion
aboutX( const float degree )
{
    return QQuaternion::fromAxisAndAngle( 1.0f, 0.0f, 0.0f, degree );
}

/*!
  \brief the longest run of true, going round from the end to the start
  \param down by sample
  \param first its first sample
  \return its length (0: none; the size: all)
*/
int
longestRun( const std::vector< bool > & down,
            int * first )
{
    const int n = static_cast< int >( down.size() );
    int best = 0;
    *first = 0;
    for ( int i = 0; i < n; ++i )
    {
        // runs start where the one before is not down
        if ( ! down[i] || down[( i + n - 1 ) % n] ) continue;
        int len = 0;
        while ( len < n && down[( i + len ) % n] ) ++len;
        if ( len > best )
        {
            best = len;
            *first = i;
        }
    }
    if ( best == 0 && n > 0 && down[0] ) best = n; // all down: no run starts
    return best;
}

}

namespace {

//! a file's name as told
QString
fileOf( const QString & path )
{
    return QFileInfo( path ).fileName();
}

//! the names of a list, quoted, joined
QString
namesOf( const QStringList & names )
{
    QStringList quoted;
    for ( const QString & n : names ) quoted << QLatin1Char( '"' ) + n + QLatin1Char( '"' );
    return quoted.join( QStringLiteral( ", " ) );
}

//! a model facing farther than this (degree) from +z is turned to face it
constexpr float FACING_TOLERANCE = 2.0f;

/*!
  \brief the roles of a model's bones (by a pack's map, a preset, or found:
  findHumanBones()), the model turned upright (+y up) if it is not (a BVH
  file written z up), and to face +z if it does not (a VRM 0.x model faces
  -z); a motion capture settled (settleCapture())
*/
bool
rigModel( const QString & path,
          const QString & skeleton,
          const std::vector< std::pair< QString, QString > > & bones,
          GltfModel & model,
          HumanoidRig & rig,
          QString * error )
{
    HumanBoneNodes nodes;
    QString why;
    if ( ! findHumanBones( model, skeleton, bones, &nodes, &why )
         || ! rig.build( model, nodes, &why ) )
    {
        *error = fileOf( path ) + QStringLiteral( ": " ) + why;
        return false;
    }
    const QVector3D up = rig.upAxis();
    if ( up != QVector3D( 0.0f, 1.0f, 0.0f ) )
    {
        // a quarter or a half turn, about +x (or +z for a model on its side)
        QQuaternion turn;
        if ( up.z() != 0.0f ) turn = QQuaternion::fromAxisAndAngle( 1.0f, 0.0f, 0.0f, up.z() > 0.0f ? -90.0f : 90.0f );
        else if ( up.x() != 0.0f ) turn = QQuaternion::fromAxisAndAngle( 0.0f, 0.0f, 1.0f, up.x() > 0.0f ? 90.0f : -90.0f );
        else turn = QQuaternion::fromAxisAndAngle( 1.0f, 0.0f, 0.0f, 180.0f );
        model.turnAbout( turn );
        if ( ! rig.build( model, nodes, &why ) )
        {
            *error = fileOf( path ) + QStringLiteral( ": " ) + why;
            return false;
        }
    }
    const float facing = rig.facingAngle();
    if ( std::fabs( facing ) > FACING_TOLERANCE )
    {
        model.turnAbout( QQuaternion::fromAxisAndAngle( 0.0f, 1.0f, 0.0f, -facing ) );
        if ( ! rig.build( model, nodes, &why ) )
        {
            *error = fileOf( path ) + QStringLiteral( ": " ) + why;
            return false;
        }
    }
    // a motion capture: its rest and its motion where a figure wants them
    if ( model.capture && settleCapture( model, rig ) )
    {
        if ( ! rig.build( model, nodes, &why ) )
        {
            *error = fileOf( path ) + QStringLiteral( ": " ) + why;
            return false;
        }
    }
    return true;
}

//! read a model of a pack (glTF, GLB, VRM) and its roles
bool
readModel( const QString & path,
           const QString & skeleton,
           const std::vector< std::pair< QString, QString > > & bones,
           GltfModel & model,
           HumanoidRig & rig,
           QString * error,
           const bool mesh_required = true )
{
    QString why;
    if ( ! model.loadFile( path, &why, mesh_required ) )
    {
        *error = fileOf( path ) + QStringLiteral( ": " ) + why;
        return false;
    }
    return rigModel( path, skeleton, bones, model, rig, error );
}

//! a clip taken as a root motion (not in place) if its hips go this far across it (hip heights)
constexpr float ROOT_MOTION = 0.05f;

//! a person's height for the height of the hips (the Mannequin: 1.829 m for 0.916 m)
constexpr float HEIGHT_PER_HIPS = 2.0f;

//! a model's height: its mesh's at rest, or else (a skeleton of clips, no mesh) from its hips
float
heightOf( const GltfModel & model,
          const HumanoidRig & rig )
{
    return ( model.height() > 0.0f ? model.height() : rig.hipHeight() * HEIGHT_PER_HIPS );
}

/*!
  \struct ClipSource
  \brief a file of clips of a motion, read once: its skeleton and its roles
*/
struct ClipSource {
    QString path;
    GltfModel model;
    HumanoidRig rig;
};

/*!
  read a file of clips: a skeleton (a mesh not required) whose rest, if
  saved in a pose (an FBX file without a skin), is the motion's model's of
  the same bones; its roles by the motion's skeleton, else found
*/
bool
readClips( const QString & path,
           const FigureMotionSpec & spec,
           const GltfModel & motion,
           ClipSource & source,
           QString * error )
{
    QString why;
    source.path = path;
    if ( ! source.model.loadFile( path, &why, false ) )
    {
        *error = fileOf( path ) + QStringLiteral( ": " ) + why;
        return false;
    }
    if ( source.model.posed_rest ) restAs( source.model, motion );
    if ( rigModel( path, spec.skeleton, spec.bones, source.model, source.rig, error ) ) return true;
    const QString first = *error;
    if ( ( ! spec.skeleton.isEmpty() || ! spec.bones.empty() )
         && rigModel( path, QString(), {}, source.model, source.rig, error ) )
    {
        return true;
    }
    *error = first;
    return false;
}

}

/*!
  \struct Figures3D::Motion
  \brief a motion: a model with the clips of standing, walking and running
*/
struct Figures3D::Motion {
    QString key; //!< the model and the clips: motions of the same are read once
    QString source; //!< the model's file
    GltfModel model;
    HumanoidRig rig; //!< its roles
    int idle = -1; //!< the clip of standing
    std::array< std::vector< GaitClip >, 4 > ways; //!< walking and running by the way (GaitWays::Way), slowest first: wayClips()
    GaitWays strides; //!< their strides
    ActionClips actions; //!< the clips of the actions, if told
};

/*!
  \struct Figures3D::Body
  \brief a body to draw, for a motion
*/
struct Figures3D::Body {
    FigureBodySpec spec; //!< as the pack tells: the materials' paints, the hairstyles
    GltfModel model;
    HumanoidRig rig; //!< its roles (on model: the body is not moved)
    //! from each limb's rest to hanging down, in the model: left, right
    QQuaternion leg_down[2];
    QQuaternion arm_down[2];
    std::vector< int > motion_node; //!< by node index: the motion's node of its name, or -1
    bool motion_skeleton = false; //!< the motion's own model: its nodes taken as they are
    //! a gait cycle of the body is this many times the motion's for the same height (longer legs: longer strides)
    float stride = 1.0f;
    FigureUniform::Cuts cuts; //!< where the uniform ends
};

/*!
  \struct Figures3D::Pack
  \brief a pack read: its motion and its bodies
*/
struct Figures3D::Pack {
    FigurePack spec;
    const Motion * motion = nullptr;
    std::vector< std::unique_ptr< Body > > bodies;
    FigureCast cast;
};

/*-------------------------------------------------------------------*/
Figures3D::Figures3D() = default;
Figures3D::~Figures3D() = default;

/*-------------------------------------------------------------------*/
bool
Figures3D::load()
{
    if ( M_loaded ) return M_ready;
    M_loaded = true;

    const FigurePack * pack = FigurePacks::builtin( QString::fromLatin1( PLAYER_MODEL_MANNEQUIN ) );
    if ( ! pack )
    {
        M_error = QStringLiteral( "no bundled pack " ) + PLAYER_MODEL_MANNEQUIN;
        return false;
    }
    M_ready = setPack( *pack );
    return M_ready;
}

/*-------------------------------------------------------------------*/
const Figures3D::Motion *
Figures3D::motionOf( const FigureMotionSpec & spec,
                     QString * error )
{
    QStringList parts( { spec.model, spec.skeleton } );
    for ( const auto & b : spec.bones ) parts << b.first + QLatin1Char( '=' ) + b.second;
    for ( const FigureClipSpec * c : { &spec.idle, &spec.walk, &spec.jog, &spec.sprint } )
    {
        parts << c->file + QLatin1Char( '|' ) + c->clip;
    }
    for ( const FigureClipSpec * c : spec.optionalClips() )
    {
        parts << ( c->given ? c->file + QLatin1Char( '|' ) + c->clip : QStringLiteral( "-" ) );
    }
    for ( const FigureClipSpec * c : spec.actionClips() )
    {
        parts << ( c->given
                   ? c->file + QLatin1Char( '|' ) + c->clip + QLatin1Char( '|' ) + QString::number( c->moment )
                   : QStringLiteral( "-" ) );
    }
    const QString key = parts.join( QLatin1Char( '\n' ) );
    for ( const std::unique_ptr< Motion > & m : M_motions )
    {
        if ( m->key == key ) return m.get();
    }

    std::unique_ptr< Motion > m( new Motion );
    m->key = key;
    m->source = spec.model;
    // the model of the motion: a skeleton, a mesh not required (a file of clips)
    if ( ! readModel( spec.model, spec.skeleton, spec.bones, m->model, m->rig, error, false ) )
    {
        return nullptr;
    }

    // a clip of a model by its name, or its first own (of the first count: not one of another file added)
    const auto indexOf = [error]( const GltfModel & model,
                                  const std::size_t count,
                                  const QString & path,
                                  const QString & name ) {
        const int clip = ( name.isEmpty() ? ( count == 0 ? -1 : 0 ) : model.clipIndex( name ) );
        if ( clip < 0 && name.isEmpty() )
        {
            *error = fileOf( path ) + QStringLiteral( ": no clips" );
        }
        else if ( clip < 0 )
        {
            QStringList clips;
            for ( std::size_t k = 0; k < count; ++k ) clips << model.clips[k].name;
            *error = fileOf( path ) + QStringLiteral( ": no clip \"" ) + name
                + QStringLiteral( "\" (its clips: " ) + namesOf( clips ) + QLatin1Char( ')' );
        }
        return clip;
    };
    // each clip: of the model, or of another file put on the model's
    // skeleton (retargetClip()); in place (a root motion taken off)
    std::vector< std::unique_ptr< ClipSource > > sources;
    const std::size_t own = m->model.clips.size();
    // a file of clips, read once
    const auto sourceOf = [&]( const QString & path ) -> ClipSource * {
        for ( const std::unique_ptr< ClipSource > & s : sources )
        {
            if ( s->path == path ) return s.get();
        }
        sources.emplace_back( new ClipSource );
        ClipSource * source = sources.back().get();
        return ( readClips( path, spec, m->model, *source, error ) ? source : nullptr );
    };
    const auto ownFile = [&spec]( const FigureClipSpec & c ) {
        return c.file.isEmpty() || QFileInfo( c.file ) == QFileInfo( spec.model );
    };
    const auto clipOf = [&]( const FigureClipSpec & c ) -> int {
        if ( ownFile( c ) )
        {
            const int clip = indexOf( m->model, own, spec.model, c.clip );
            if ( clip >= 0 && clipDrift( m->model, m->rig, m->model.clips[clip] ) > ROOT_MOTION )
            {
                const GltfModel::Clip own = m->model.clips[clip];
                m->model.clips[clip] = retargetClip( m->model, m->rig, own, m->model, m->rig, true );
            }
            return clip;
        }
        ClipSource * source = sourceOf( c.file );
        if ( ! source ) return -1;
        const int clip = indexOf( source->model, source->model.clips.size(), c.file, c.clip );
        if ( clip < 0 ) return -1;
        GltfModel::Clip put = retargetClip( source->model, source->rig, source->model.clips[clip], m->model, m->rig, true );
        put.name = c.text();
        m->model.clips.push_back( std::move( put ) );
        return static_cast< int >( m->model.clips.size() ) - 1;
    };
    // a clip of an action: as it goes, but pinned if it goes somewhere (a
    // slide, a dive: done where the player is)
    const auto actionOf = [&]( const FigureClipSpec & c ) -> int {
        if ( ownFile( c ) )
        {
            const int clip = indexOf( m->model, own, spec.model, c.clip );
            if ( clip >= 0 && clipTravel( m->model, m->rig, m->model.clips[clip] ) > ACTION_TRAVEL )
            {
                const GltfModel::Clip as = m->model.clips[clip];
                m->model.clips[clip] = retargetClip( m->model, m->rig, as, m->model, m->rig, ClipPlace::Pinned );
            }
            return clip;
        }
        ClipSource * source = sourceOf( c.file );
        if ( ! source ) return -1;
        const int clip = indexOf( source->model, source->model.clips.size(), c.file, c.clip );
        if ( clip < 0 ) return -1;
        const GltfModel::Clip & from = source->model.clips[clip];
        GltfModel::Clip put = retargetClip( source->model, source->rig, from, m->model, m->rig,
                                            clipTravel( source->model, source->rig, from ) > ACTION_TRAVEL
                                            ? ClipPlace::Pinned : ClipPlace::AsItIs );
        put.name = c.text();
        m->model.clips.push_back( std::move( put ) );
        return static_cast< int >( m->model.clips.size() ) - 1;
    };
    m->idle = clipOf( spec.idle );
    if ( m->idle < 0 ) return nullptr;

    // a clip of walking or running of a way: how it goes (analyzeGait())
    const auto clipText = [&spec]( const FigureClipSpec & c ) {
        return ( c.file.isEmpty() ? fileOf( spec.model ) + QStringLiteral( ": the clip " ) : QStringLiteral( "the clip " ) ) + c.text();
    };
    const auto addGait = [&]( const int clip,
                              const int way,
                              const QString & text ) {
        // the way each goes in the model: forward +z, to the left +x
        static const QVector3D travel[4] = { QVector3D( 0.0f, 0.0f, 1.0f ), QVector3D( -1.0f, 0.0f, 0.0f ),
                                             QVector3D( 0.0f, 0.0f, -1.0f ), QVector3D( 1.0f, 0.0f, 0.0f ) };
        static const char * const name[4] = { "", " to the right", " backward", " to the left" };
        GaitClip gait;
        if ( ! analyzeGait( m->model, m->rig, clip, &gait, nullptr, nullptr, travel[way] ) )
        {
            *error = text + QStringLiteral( " is not of walking or running" ) + QLatin1String( name[way] )
                + ( way == GaitWays::Forward
                    ? QStringLiteral( " (the feet do not come down and go back)" )
                    : QStringLiteral( " (the feet do not come down and go against the way)" ) );
            return false;
        }
        m->ways[way].push_back( gait );
        return true;
    };
    for ( const FigureClipSpec * c : { &spec.walk, &spec.jog, &spec.sprint } )
    {
        const int clip = clipOf( *c );
        if ( clip < 0 || ! addGait( clip, GaitWays::Forward, clipText( *c ) ) ) return nullptr;
    }

    // the clips of the other ways, if told: backward; sideways, a side not
    // told the other side's mirrored
    for ( const FigureClipSpec * c : { &spec.walk_back, &spec.jog_back } )
    {
        if ( ! c->given ) continue;
        const int clip = clipOf( *c );
        if ( clip < 0 || ! addGait( clip, GaitWays::Backward, clipText( *c ) ) ) return nullptr;
    }
    const auto mirrored = [&]( const int clip ) {
        GltfModel::Clip mirror = mirrorClip( m->model, m->rig, m->model.clips[clip] );
        mirror.name = m->model.clips[clip].name + QStringLiteral( " (mirrored)" );
        m->model.clips.push_back( std::move( mirror ) );
        return static_cast< int >( m->model.clips.size() ) - 1;
    };
    for ( const std::pair< const FigureClipSpec *, const FigureClipSpec * > & side
              : { std::make_pair( &spec.walk_left, &spec.walk_right ), std::make_pair( &spec.jog_left, &spec.jog_right ) } )
    {
        const FigureClipSpec & left = *side.first;
        const FigureClipSpec & right = *side.second;
        if ( ! left.given && ! right.given ) continue;
        int l = ( left.given ? clipOf( left ) : -1 );
        int r = ( right.given ? clipOf( right ) : -1 );
        if ( ( left.given && l < 0 ) || ( right.given && r < 0 ) ) return nullptr;
        const QString l_text = ( left.given ? clipText( left ) : clipText( right ) + QStringLiteral( " mirrored" ) );
        const QString r_text = ( right.given ? clipText( right ) : clipText( left ) + QStringLiteral( " mirrored" ) );
        if ( l < 0 ) l = mirrored( r );
        if ( r < 0 ) r = mirrored( l );
        if ( ! addGait( l, GaitWays::Left, l_text )
             || ! addGait( r, GaitWays::Right, r_text ) )
        {
            return nullptr;
        }
    }
    for ( std::vector< GaitClip > & clips : m->ways )
    {
        std::sort( clips.begin(), clips.end(),
                   []( const GaitClip & a, const GaitClip & b ) { return a.speed < b.speed; } );
    }
    // backward, with clips of other ways: then the forward ones played
    // backward, faster than the backward ones
    std::vector< GaitClip > & back = m->ways[GaitWays::Backward];
    if ( ! back.empty() || ! m->ways[GaitWays::Left].empty() )
    {
        const float fastest = ( back.empty() ? -1.0f : back.back().speed );
        for ( const GaitClip & g : m->ways[GaitWays::Forward] )
        {
            if ( g.speed <= fastest ) continue;
            GaitClip reversed = g;
            reversed.reverse = true;
            back.push_back( reversed );
        }
    }
    for ( std::size_t w = 0; w < m->ways.size(); ++w )
    {
        for ( const GaitClip & g : m->ways[w] ) m->strides.strides[w].push_back( { g.speed, g.stride() } );
    }

    // the clips of the actions, if told
    ActionClips & acts = m->actions;
    const auto actionClip = [&]( const int clip ) {
        ActionClip a;
        a.clip = clip;
        a.duration = m->model.clips[clip].duration;
        a.down = downTime( m->model, m->rig, clip );
        return a;
    };
    // a kick: with its foot, and the other foot's mirrored
    const auto kickOf = [&]( const FigureClipSpec & c,
                             ActionClip & a ) {
        if ( ! c.given ) return true;
        const int clip = actionOf( c );
        if ( clip < 0 ) return false;
        float moment = 0.0f;
        bool right = true;
        if ( ! kickMoment( m->model, m->rig, clip, &moment, &right ) && c.moment < 0.0f )
        {
            *error = clipText( c ) + QStringLiteral( " is not of kicking (no foot swings forward)" );
            return false;
        }
        a = actionClip( clip );
        a.moment = ( c.moment >= 0.0f ? c.moment : moment );
        const int mirror = mirrored( clip );
        a.clip = ( right ? clip : mirror );
        a.other = ( right ? mirror : clip );
        return true;
    };
    ActionClip kick;
    if ( ! kickOf( spec.kick, kick )
         || ! kickOf( spec.kick_tap, acts.kick[static_cast< int >( KickKind::Tap )] )
         || ! kickOf( spec.kick_pass, acts.kick[static_cast< int >( KickKind::Pass )] )
         || ! kickOf( spec.kick_shot, acts.kick[static_cast< int >( KickKind::Shot )] ) )
    {
        return nullptr;
    }
    for ( const KickKind k : { KickKind::Pass, KickKind::Shot } )
    {
        if ( ! acts.kick[static_cast< int >( k )].valid() ) acts.kick[static_cast< int >( k )] = kick;
    }
    // going down to the ground: the moment on it
    const auto fallOf = [&]( const FigureClipSpec & c,
                             ActionClip & a ) {
        if ( ! c.given ) return true;
        const int clip = actionOf( c );
        if ( clip < 0 ) return false;
        a = actionClip( clip );
        if ( a.down < 0.0f && c.moment < 0.0f )
        {
            *error = clipText( c ) + QStringLiteral( " is not of falling (the hips do not come down)" );
            return false;
        }
        a.moment = ( c.moment >= 0.0f ? c.moment : a.down );
        if ( a.down < 0.0f ) a.down = a.moment;
        return true;
    };
    // getting up: as it goes; a catch: the moment the hands reach farthest
    const auto plainOf = [&]( const FigureClipSpec & c,
                              ActionClip & a,
                              const bool reach ) {
        if ( ! c.given ) return true;
        const int clip = actionOf( c );
        if ( clip < 0 ) return false;
        a = actionClip( clip );
        a.moment = ( c.moment >= 0.0f ? c.moment : reach ? reachMoment( m->model, m->rig, clip ) : 0.0f );
        return true;
    };
    if ( ! fallOf( spec.tackle, acts.tackle )
         || ! fallOf( spec.fall, acts.fall )
         || ! plainOf( spec.tackle_up, acts.tackle_up, false )
         || ! plainOf( spec.get_up, acts.get_up, false )
         || ! plainOf( spec.catch_low, acts.catch_low, true )
         || ! plainOf( spec.catch_high, acts.catch_high, true ) )
    {
        return nullptr;
    }
    // a dive: to the right, and to the left (a side not told: the other's mirrored)
    if ( spec.dive_left.given || spec.dive_right.given )
    {
        ActionClip left;
        ActionClip right;
        if ( ! plainOf( spec.dive_left, left, true )
             || ! plainOf( spec.dive_right, right, true ) )
        {
            return nullptr;
        }
        acts.dive = ( right.valid() ? right : left );
        acts.dive.clip = ( right.valid() ? right.clip : mirrored( left.clip ) );
        acts.dive.other = ( left.valid() ? left.clip : mirrored( right.clip ) );
        if ( acts.dive.down < 0.0f ) acts.dive.down = acts.dive.duration;
    }

    M_motions.push_back( std::move( m ) );
    return M_motions.back().get();
}

/*-------------------------------------------------------------------*/
std::unique_ptr< Figures3D::Body >
Figures3D::loadBody( const FigureBodySpec & spec,
                     const Motion & motion,
                     QString * error ) const
{
    std::unique_ptr< Body > body( new Body );
    body->spec = spec;
    if ( spec.model == motion.source )
    {
        // the motion's own model
        body->model = motion.model;
        body->motion_skeleton = true;
        if ( ! rigModel( spec.model, spec.skeleton, spec.bones, body->model, body->rig, error ) )
        {
            return nullptr;
        }
    }
    else if ( ! readModel( spec.model, spec.skeleton, spec.bones, body->model, body->rig, error ) )
    {
        return nullptr;
    }

    // the materials that the pack paints are the model's
    QStringList materials;
    for ( const SkinPrimitive & p : body->model.primitives )
    {
        if ( ! materials.contains( p.material ) ) materials << p.material;
    }
    QStringList named = spec.team + spec.uniform + spec.hair + spec.skin + spec.tint + spec.hairstyles;
    for ( const FigureBodySpec::Extra & e : spec.extras ) named << e.material;
    for ( const QString & m : named )
    {
        if ( ! materials.contains( m ) )
        {
            *error = fileOf( spec.model ) + QStringLiteral( ": no material \"" ) + m
                + QStringLiteral( "\" (its materials: " ) + namesOf( materials ) + QLatin1Char( ')' );
            return nullptr;
        }
    }

    // each limb from its rest (in the T pose: HumanoidRig::restTurn()) to hanging down
    std::vector< QMatrix4x4 > global;
    body->model.globals( body->model.rest, global );
    const HumanoidRig & rig = body->rig;
    auto down = [&]( const HumanBone upper,
                     const HumanBone lower )
        {
            if ( rig.node( upper ) < 0 || rig.node( lower ) < 0 ) return QQuaternion();
            const QVector3D rest = ( global[rig.node( lower )].column( 3 ) - global[rig.node( upper )].column( 3 ) ).toVector3D();
            const QQuaternion turn = rig.restTurn( upper );
            const QVector3D dir = ( turn.isIdentity() ? rest : turn.rotatedVector( rest ) );
            return QQuaternion::rotationTo( dir.normalized(), QVector3D( 0.0f, -1.0f, 0.0f ) );
        };
    body->leg_down[0] = down( HumanBone::LeftUpperLeg, HumanBone::LeftLowerLeg );
    body->leg_down[1] = down( HumanBone::RightUpperLeg, HumanBone::RightLowerLeg );
    body->arm_down[0] = down( HumanBone::LeftUpperArm, HumanBone::LeftLowerArm );
    body->arm_down[1] = down( HumanBone::RightUpperArm, HumanBone::RightLowerArm );

    // the bones of the motion by their names
    for ( const QString & name : body->model.node_names )
    {
        body->motion_node.push_back( motion.model.nodeIndex( name ) );
    }
    // the strides: the gait clips read on the body, of the same height
    // (longer legs, longer strides), their mean of the motion's
    if ( ! body->motion_skeleton )
    {
        float sum = 0.0f;
        int count = 0;
        for ( const GaitClip & g : motion.ways[GaitWays::Forward] )
        {
            GaitClip on_body;
            if ( g.speed > 0.0f
                 && analyzeGait( motion.model, motion.rig, g.clip, &on_body, &body->model, &rig ) )
            {
                sum += on_body.speed / g.speed;
                ++count;
            }
        }
        if ( count > 0 ) body->stride = sum / count;
    }

    // the uniform, and the skin under the hairstyles and the extras
    if ( ! spec.uniform.isEmpty() )
    {
        FigureUniform::paintParts( body->model, rig );
        body->cuts = FigureUniform::cutsOf( body->model, rig, spec.uniform );
        std::vector< std::pair< QString, std::uint32_t > > hairs;
        for ( const SkinPrimitive & p : body->model.primitives )
        {
            const std::uint32_t bit = spec.bitOf( p.material );
            if ( bit != 0
                 && std::none_of( hairs.begin(), hairs.end(),
                                  [&p]( const std::pair< QString, std::uint32_t > & h ) { return h.first == p.material; } ) )
            {
                hairs.emplace_back( p.material, bit );
            }
        }
        FigureUniform::paintScalp( body->model, spec.uniform, hairs, &body->cuts );
    }
    return body;
}

/*-------------------------------------------------------------------*/
bool
Figures3D::setPack( const FigurePack & pack )
{
    if ( ! pack.ok() )
    {
        M_error = pack.displayName() + QStringLiteral( ": " ) + pack.error;
        return false;
    }
    for ( const std::unique_ptr< Pack > & p : M_packs )
    {
        if ( p->spec.id == pack.id && p->spec.dir == pack.dir )
        {
            M_pack = p.get();
            return true;
        }
    }

    std::unique_ptr< Pack > p( new Pack );
    p->spec = pack;
    QString why;
    p->motion = motionOf( pack.motion, &why );
    if ( ! p->motion )
    {
        M_error = pack.displayName() + QStringLiteral( ": " ) + why;
        return false;
    }
    for ( const FigureBodySpec & spec : pack.bodies )
    {
        std::unique_ptr< Body > body = loadBody( spec, *p->motion, &why );
        if ( ! body )
        {
            M_error = pack.displayName() + QStringLiteral( ": " ) + why;
            return false;
        }
        p->bodies.push_back( std::move( body ) );
    }
    p->cast = pack.cast();
    M_pack = p.get();
    M_packs.push_back( std::move( p ) );
    return true;
}

/*-------------------------------------------------------------------*/
const FigurePack &
Figures3D::pack() const
{
    static const FigurePack none = FigurePack();
    return ( M_pack ? M_pack->spec : none );
}

/*-------------------------------------------------------------------*/
const FigureCast &
Figures3D::cast() const
{
    static const FigureCast none = FigureCast();
    return ( M_pack ? M_pack->cast : none );
}

/*-------------------------------------------------------------------*/
const Figures3D::Motion &
Figures3D::motion() const
{
    static const Motion none = Motion();
    return ( M_pack ? *M_pack->motion : none );
}

/*-------------------------------------------------------------------*/
const GltfModel &
Figures3D::motionModel() const
{
    return motion().model;
}

/*-------------------------------------------------------------------*/
const HumanoidRig &
Figures3D::motionRig() const
{
    return motion().rig;
}

/*-------------------------------------------------------------------*/
const std::vector< GaitClip > &
Figures3D::gaitClips() const
{
    return motion().ways[GaitWays::Forward];
}

/*-------------------------------------------------------------------*/
const std::vector< GaitClip > &
Figures3D::wayClips( const int way ) const
{
    static const std::vector< GaitClip > none;
    return ( way >= 0 && way < 4 ? motion().ways[way] : none );
}

/*-------------------------------------------------------------------*/
GaitWays
Figures3D::gaitWays() const
{
    return motion().strides;
}

/*-------------------------------------------------------------------*/
int
Figures3D::bodyCount() const
{
    return ( M_pack ? static_cast< int >( M_pack->bodies.size() ) : 0 );
}

/*-------------------------------------------------------------------*/
int
Figures3D::bodyOf( const FigureInstance & figure ) const
{
    return std::min( bodyCount() - 1, std::max( 0, figure.look.body ) );
}

/*-------------------------------------------------------------------*/
const Figures3D::Body &
Figures3D::bodyFor( const FigureInstance & figure ) const
{
    return *M_pack->bodies[bodyOf( figure )];
}

/*-------------------------------------------------------------------*/
const GltfModel &
Figures3D::model( const int body ) const
{
    return ( body >= 0 && body < bodyCount() ? M_pack->bodies[body]->model : motionModel() );
}

/*-------------------------------------------------------------------*/
const HumanoidRig &
Figures3D::rig( const int body ) const
{
    return ( body >= 0 && body < bodyCount() ? M_pack->bodies[body]->rig : motionRig() );
}

/*-------------------------------------------------------------------*/
float
Figures3D::bodyStride( const int body ) const
{
    return ( body >= 0 && body < bodyCount() ? M_pack->bodies[body]->stride : 1.0f );
}

/*-------------------------------------------------------------------*/
QStringList
Figures3D::jerseyMaterials( const int body ) const
{
    return ( body >= 0 && body < bodyCount() ? M_pack->bodies[body]->spec.team : QStringList() );
}

/*-------------------------------------------------------------------*/
QStringList
Figures3D::uniformMaterials( const int body ) const
{
    return ( body >= 0 && body < bodyCount() ? M_pack->bodies[body]->spec.uniform : QStringList() );
}

/*-------------------------------------------------------------------*/
std::vector< FigureUniform::Paint >
Figures3D::paints( const int body ) const
{
    std::vector< FigureUniform::Paint > paints;
    if ( body < 0 || body >= bodyCount() ) return paints;
    const Body & b = *M_pack->bodies[body];
    for ( const SkinPrimitive & p : b.model.primitives )
    {
        FigureUniform::Paint paint;
        if ( b.spec.uniform.contains( p.material ) )
        {
            paint.kind = FigureUniform::Paint::Uniform;
        }
        else if ( b.spec.team.contains( p.material ) )
        {
            paint.kind = FigureUniform::Paint::Figure;
        }
        else if ( b.spec.hair.contains( p.material ) )
        {
            // the eyebrows, on all
            paint.kind = FigureUniform::Paint::Hair;
        }
        else if ( b.spec.skin.contains( p.material ) )
        {
            paint.kind = FigureUniform::Paint::Skin;
        }
        else if ( b.spec.tint.contains( p.material ) )
        {
            paint.kind = FigureUniform::Paint::Tint;
        }
        else if ( const std::uint32_t bit = b.spec.bitOf( p.material ) )
        {
            // a hairstyle or an extra, on those wearing it
            paint.kind = FigureUniform::Paint::Hair;
            paint.show = bit;
        }
        paints.push_back( paint );
    }
    return paints;
}

/*-------------------------------------------------------------------*/
const FigureUniform::Cuts &
Figures3D::uniformCuts( const int body ) const
{
    static const FigureUniform::Cuts none = FigureUniform::Cuts();
    return ( body >= 0 && body < bodyCount() ? M_pack->bodies[body]->cuts : none );
}

/*-------------------------------------------------------------------*/
bool
Figures3D::analyzeGait( const GltfModel & model,
                        const HumanoidRig & rig,
                        const int clip,
                        GaitClip * out,
                        const GltfModel * body,
                        const HumanoidRig * body_rig,
                        const QVector3D & travel )
{
    if ( clip < 0
         || clip >= static_cast< int >( model.clips.size() )
         || model.clips[clip].duration <= 0.0f
         || heightOf( model, rig ) <= 0.0f
         || ( body && ( ! body_rig || heightOf( *body, *body_rig ) <= 0.0f ) ) )
    {
        return false;
    }
    const GltfModel::Clip & c = model.clips[clip];
    // the feet of the model, or of the body with the clip's pose
    const GltfModel & feet_model = ( body ? *body : model );
    const HumanoidRig & feet_rig = ( body ? *body_rig : rig );
    const HumanBone feet[2] = { HumanBone::LeftFoot, HumanBone::RightFoot };
    const HumanBone toes[2] = { HumanBone::LeftToes, HumanBone::RightToes };

    // the ankles and the toes over the cycle (how far along the way gone)
    std::vector< float > ankle_y[2], toe_y[2], along[2];
    std::vector< NodeTransform > local, body_local;
    std::vector< QMatrix4x4 > global;
    for ( int k = 0; k < GAIT_SAMPLES; ++k )
    {
        local = model.rest;
        model.sample( c, c.duration * k / GAIT_SAMPLES, local );
        if ( body )
        {
            body_local = body->rest;
            body_rig->apply( rig.poseOf( local ), body_local );
            body->globals( body_local, global );
        }
        else
        {
            model.globals( local, global );
        }
        for ( int s = 0; s < 2; ++s )
        {
            const QVector3D ankle = global[feet_rig.node( feet[s] )].column( 3 ).toVector3D();
            const QVector3D toe = ( feet_rig.node( toes[s] ) >= 0
                                    ? global[feet_rig.node( toes[s] )].column( 3 ).toVector3D()
                                    : ankle );
            ankle_y[s].push_back( ankle.y() );
            toe_y[s].push_back( toe.y() );
            along[s].push_back( QVector3D::dotProduct( ( ankle + toe ) * 0.5f, travel ) );
        }
    }

    // A foot is down while its ankle or its toes are near their lowest.
    // The longest such run: from the foot coming down to it leaving.
    float speed_sum = 0.0f;
    float contact = 0.0f;
    float down_time = 0.0f;
    for ( int s = 0; s < 2; ++s )
    {
        const float ankle_min = *std::min_element( ankle_y[s].begin(), ankle_y[s].end() );
        const float toe_min = *std::min_element( toe_y[s].begin(), toe_y[s].end() );
        std::vector< float > h( GAIT_SAMPLES );
        for ( int k = 0; k < GAIT_SAMPLES; ++k )
        {
            h[k] = std::min( ankle_y[s][k] - ankle_min, toe_y[s][k] - toe_min );
        }
        const float high = *std::max_element( h.begin(), h.end() );
        if ( high < FOOT_LIFT * heightOf( feet_model, feet_rig ) ) return false; // not lifted: not walking
        std::vector< bool > down( GAIT_SAMPLES );
        for ( int k = 0; k < GAIT_SAMPLES; ++k ) down[k] = ( h[k] < FOOT_DOWN * high );
        int first = 0;
        int len = longestRun( down, &first );
        if ( len < 4 || len >= GAIT_SAMPLES ) return false;

        // how fast it goes back while low: the median over the run, so that
        // the slow parts where it comes down and leaves count little
        const double dt = c.duration / GAIT_SAMPLES;
        auto back = [&]( const int i )
            {
                const int k = ( first + i ) % GAIT_SAMPLES;
                const double before = along[s][( k + GAIT_SAMPLES - 1 ) % GAIT_SAMPLES];
                const double after = along[s][( k + 1 ) % GAIT_SAMPLES];
                return ( before - after ) / ( 2.0 * dt );
            };
        auto median = [&]( const int from, const int count )
            {
                std::vector< double > v;
                for ( int i = from; i < from + count; ++i ) v.push_back( back( i ) );
                std::nth_element( v.begin(), v.begin() + v.size() / 2, v.end() );
                return v[v.size() / 2];
            };
        const double low_speed = median( 0, len );
        if ( low_speed <= 0.0 ) return false; // not going back

        // On the ground: the longest part of the run going back at half of
        // that or faster. A foot may come low before it lands, hardly going
        // back (a sprint's): not yet on the ground.
        int best_from = 0, best_len = 0;
        for ( int i = 0; i < len; )
        {
            int j = i;
            while ( j < len && back( j ) >= FOOT_BACK * low_speed ) ++j;
            if ( j - i > best_len )
            {
                best_from = i;
                best_len = j - i;
            }
            i = std::max( j, i + 1 );
        }
        if ( best_len < 2 ) return false;
        const double speed = median( best_from, best_len );
        first = ( first + best_from ) % GAIT_SAMPLES;
        len = best_len;

        speed_sum += static_cast< float >( speed );
        if ( s == 0 )
        {
            contact = c.duration * first / GAIT_SAMPLES;
            down_time = c.duration * len / GAIT_SAMPLES;
        }
    }

    out->clip = clip;
    out->contact = contact;
    out->down = down_time;
    out->duration = c.duration;
    out->speed = speed_sum * 0.5f * SceneBuilder::PERSON_HEIGHT / heightOf( feet_model, feet_rig );
    return true;
}

/*-------------------------------------------------------------------*/
std::vector< GaitStride >
Figures3D::strides() const
{
    std::vector< GaitStride > strides;
    for ( const GaitClip & g : gaitClips() )
    {
        strides.push_back( { g.speed, g.stride() } );
    }
    return strides;
}

/*-------------------------------------------------------------------*/
Figures3D::Mix
Figures3D::mixAt( const float speed ) const
{
    const std::vector< GaitClip > & gaits = gaitClips();
    Mix mix = mixOf( gaits, speed );
    if ( ! gaits.empty() ) mix.moving = clamp01( speed / ( MOVING_FULL * gaits.front().speed ) );
    return mix;
}

/*-------------------------------------------------------------------*/
Figures3D::Mix
Figures3D::mixOf( const std::vector< GaitClip > & gaits,
                  const float speed )
{
    Mix mix;
    if ( gaits.empty() ) return mix;
    const int last = static_cast< int >( gaits.size() ) - 1;
    mix.from = mix.to = last; // faster than all: the fastest
    for ( int k = 0; k <= last; ++k )
    {
        if ( speed <= gaits[k].speed )
        {
            // at a clip's speed or below the slowest: that clip; else from the one before
            mix.to = k;
            mix.from = ( k == 0 || speed == gaits[k].speed ? k : k - 1 );
            if ( mix.from != mix.to )
            {
                mix.t = ( speed - gaits[k - 1].speed ) / ( gaits[k].speed - gaits[k - 1].speed );
            }
            break;
        }
    }
    return mix;
}

/*-------------------------------------------------------------------*/
Figures3D::Gait
Figures3D::gaitOf( const FigureInstance & figure ) const
{
    Gait gait;
    const Motion & m = motion();
    if ( m.ways[GaitWays::Forward].empty() ) return gait;
    const float speed = std::hypot( figure.motion.vx, figure.motion.vy );
    const float toward = ( speed < 1.0e-3f
                           ? 0.0f
                           : std::atan2( figure.motion.vy, figure.motion.vx ) - figure.body * float( M_PI ) / 180.0f );
    gait.ways = m.strides.mixAt( speed, toward );
    const std::vector< GaitClip > & a = wayClips( gait.ways.a );
    const std::vector< GaitClip > & b = wayClips( gait.ways.b );
    gait.a = mixOf( a, speed );
    gait.b = mixOf( b, speed );
    // standing mixed in below the slowest of the ways mixed
    const float slowest = a.front().speed + ( b.front().speed - a.front().speed ) * gait.ways.t;
    gait.moving = clamp01( speed / ( MOVING_FULL * slowest ) );
    return gait;
}

/*-------------------------------------------------------------------*/
float
Figures3D::twistOf( const FigureInstance & figure ) const
{
    const float speed = std::hypot( figure.motion.vx, figure.motion.vy );
    if ( speed < 1.0e-3f ) return 0.0f;
    const float toward = std::atan2( figure.motion.vy, figure.motion.vx ) - figure.body * float( M_PI ) / 180.0f;
    const GaitWays & ways = motion().strides;
    if ( ways.sideways() )
    {
        // up to GaitWays::TWIST_FULL from forward or backward, the clips
        // mixed for the rest (as the gait table takes their strides)
        return ways.twistAt( toward ) * 180.0f / float( M_PI ) * gaitOf( figure ).moving;
    }
    // toward 45 degrees from the body, the hips turn as far: the most;
    // toward 90 (sideways, where the gait turns between forward and
    // backward), not at all, so that they do not jump there
    return TWIST_MAX * std::sin( 2.0f * toward ) * gaitOf( figure ).moving;
}

/*-------------------------------------------------------------------*/
QMatrix4x4
Figures3D::modelMatrix( const FigureInstance & figure ) const
{
    return modelMatrix( model( bodyOf( figure ) ), figure );
}

/*-------------------------------------------------------------------*/
QMatrix4x4
Figures3D::modelMatrix( const GltfModel & body,
                        const FigureInstance & figure ) const
{
    // A glTF person faces +z. The field's angle d (from +x toward +y, which
    // is the world's +z) is +x turned about y by -d, and +z turned by 90 - d.
    QMatrix4x4 m;
    m.translate( figure.x, 0.0f, figure.y );
    m.rotate( 90.0f - figure.body, 0.0f, 1.0f, 0.0f );

    // leaning toward the acceleration, about the feet: in the model, its
    // forward is +z and its left +x; leaning turns +y toward them
    const float ax = figure.motion.ax;
    const float ay = figure.motion.ay;
    const float accel = std::hypot( ax, ay );
    if ( accel > 1.0e-3f )
    {
        const float rad = figure.body * float( M_PI ) / 180.0f;
        const float forward = ax * std::cos( rad ) + ay * std::sin( rad );
        const float left = ax * std::sin( rad ) - ay * std::cos( rad );
        m.rotate( TILT_MAX * std::tanh( accel / TILT_ACCELERATION ),
                  forward / accel, 0.0f, -left / accel );
    }
    // to a ball to catch to the side: +y turned toward the right, -x
    const float roll = rollOf( figure );
    if ( roll != 0.0f )
    {
        m.rotate( roll, 0.0f, 0.0f, 1.0f );
    }

    m.scale( figure.height / std::max( 0.01f, body.height() ) );
    m.translate( 0.0f, -body.min_y, 0.0f ); // the feet on the ground
    return m;
}

/*-------------------------------------------------------------------*/
float
Figures3D::curve( std::initializer_list< std::pair< float, float > > points,
                  const float x )
{
    if ( points.size() == 0 ) return 0.0f;
    const std::pair< float, float > * p = points.begin();
    if ( x <= p[0].first ) return p[0].second;
    for ( std::size_t i = 1; i < points.size(); ++i )
    {
        if ( x <= p[i].first )
        {
            const float t = ( x - p[i - 1].first ) / std::max( 1.0e-6f, p[i].first - p[i - 1].first );
            const float eased = t * t * ( 3.0f - 2.0f * t );
            return p[i - 1].second + ( p[i].second - p[i - 1].second ) * eased;
        }
    }
    return p[points.size() - 1].second;
}

/*-------------------------------------------------------------------*/
Figures3D::Weights
Figures3D::weightsOf( const FigureActions & actions )
{
    using namespace FigureActionLog;
    Weights w;
    const FigureKick & kick = actions.kick;
    if ( kick.on )
    {
        const int k = static_cast< int >( kick.kind );
        const float top = ( kick.kind == KickKind::Tap ? KICK_TAP_WEIGHT : 1.0f ); // a touch keeps some of the running
        w.kick = curve( { { -KICK_LEAD[k], 0.0f },
                          { -0.7f * KICK_LEAD[k], top },
                          { KICK_TAIL[k] - 0.5f, top },
                          { KICK_TAIL[k], 0.0f } }, kick.phase );
    }
    if ( actions.tackle.on )
    {
        const float last = actions.tackle.last;
        w.tackle = curve( { { -TACKLE_LEAD, 0.0f }, { 0.0f, 1.0f },
                            { last + TACKLE_TAIL - 1.2f, 1.0f }, { last + TACKLE_TAIL, 0.0f } },
                          actions.tackle.phase );
    }
    if ( actions.fouled.on )
    {
        const float last = actions.fouled.last;
        // down from after the tackler slid in, while the server holds it
        const float down = FOULED_START + FOULED_FALL;
        w.fouled = curve( { { FOULED_START, 0.0f }, { down, 1.0f },
                            { std::max( down, last + FOULED_TAIL - 1.3f ), 1.0f },
                            { std::max( down + 0.1f, last + FOULED_TAIL ), 0.0f } },
                          actions.fouled.phase );
    }
    if ( actions.caught.on )
    {
        const float hold = actions.caught.hold;
        w.caught = curve( { { -CATCH_LEAD, 0.0f }, { -0.3f, 1.0f },
                            { hold + CATCH_TAIL - 1.0f, 1.0f }, { hold + CATCH_TAIL, 0.0f } },
                          actions.caught.phase );
    }
    return w;
}

/*-------------------------------------------------------------------*/
float
Figures3D::rollOf( const FigureInstance & figure ) const
{
    const FigureCatch & c = figure.actions.caught;
    if ( ! c.on || figure.actions.fouled.on ) return 0.0f;
    // a ball more than half a meter to the side: leaning to it, most from 2 m
    const float reach = clamp01( ( std::fabs( c.side ) - 0.5f ) / 1.5f );
    // a dive's clip lays the body down to the side itself
    return weightsOf( figure.actions ).caught * reach * CATCH_ROLL * ( c.side < 0.0f ? -1.0f : 1.0f )
        * ( 1.0f - diveOf( figure.actions ) );
}

/*-------------------------------------------------------------------*/
const ActionClips &
Figures3D::actionClips() const
{
    return motion().actions;
}

namespace {

//! samples a second of a clip read for an action
constexpr float ACTION_SAMPLES = 60.0f;

//! the times of an action's clip, at ACTION_SAMPLES a second (the last one kept in it)
std::vector< float >
actionTimes( const GltfModel::Clip & clip )
{
    const int steps = std::max( 2, static_cast< int >( std::ceil( clip.duration * ACTION_SAMPLES - 1.0e-3f ) ) );
    ActionClip a;
    a.duration = clip.duration;
    std::vector< float > times;
    for ( int k = 0; k <= steps; ++k ) times.push_back( a.at( clip.duration * k / steps ) );
    return times;
}

}

/*-------------------------------------------------------------------*/
bool
Figures3D::kickMoment( const GltfModel & model,
                       const HumanoidRig & rig,
                       const int clip,
                       float * moment,
                       bool * right )
{
    const GltfModel::Clip & c = model.clips[clip];
    // the toes, or else the foot, of each leg
    int feet[2];
    for ( int r = 0; r < 2; ++r )
    {
        const int toes = rig.node( r ? HumanBone::RightToes : HumanBone::LeftToes );
        feet[r] = ( toes >= 0 ? toes : rig.node( r ? HumanBone::RightFoot : HumanBone::LeftFoot ) );
    }
    const std::vector< float > times = actionTimes( c );
    std::vector< NodeTransform > local;
    std::vector< QMatrix4x4 > global;
    float before[2] = { 0.0f, 0.0f };
    float fastest = -1.0f;
    *moment = 0.0f;
    *right = true;
    for ( std::size_t k = 0; k < times.size(); ++k )
    {
        local = model.rest;
        model.sample( c, times[k], local );
        model.globals( local, global );
        for ( int r = 0; r < 2; ++r )
        {
            const float z = global[feet[r]].column( 3 ).z();
            if ( k > 0 && times[k] > times[k - 1] )
            {
                // forward: +z in the model
                const float speed = ( z - before[r] ) / ( times[k] - times[k - 1] ) / rig.hipHeight();
                if ( speed > fastest )
                {
                    fastest = speed;
                    *moment = 0.5f * ( times[k] + times[k - 1] );
                    *right = ( r == 1 );
                }
            }
            before[r] = z;
        }
    }
    return fastest >= KICK_SPEED;
}

/*-------------------------------------------------------------------*/
float
Figures3D::downTime( const GltfModel & model,
                     const HumanoidRig & rig,
                     const int clip )
{
    const GltfModel::Clip & c = model.clips[clip];
    std::vector< NodeTransform > local;
    for ( const float t : actionTimes( c ) )
    {
        local = model.rest;
        model.sample( c, t, local );
        // the hips' place from their rest, in hip heights
        if ( rig.poseOf( local ).hips.y() <= DOWN_HIPS - 1.0f ) return t;
    }
    return -1.0f;
}

/*-------------------------------------------------------------------*/
float
Figures3D::reachMoment( const GltfModel & model,
                        const HumanoidRig & rig,
                        const int clip )
{
    const GltfModel::Clip & c = model.clips[clip];
    const int chest = rig.node( rig.node( HumanBone::UpperChest ) >= 0 ? HumanBone::UpperChest : HumanBone::Chest );
    const int hands[2] = { rig.node( HumanBone::LeftHand ), rig.node( HumanBone::RightHand ) };
    std::vector< NodeTransform > local;
    std::vector< QMatrix4x4 > global;
    float farthest = -1.0f;
    float moment = 0.0f;
    for ( const float t : actionTimes( c ) )
    {
        local = model.rest;
        model.sample( c, t, local );
        model.globals( local, global );
        const QVector3D reach = 0.5f * ( global[hands[0]].column( 3 ).toVector3D() + global[hands[1]].column( 3 ).toVector3D() )
            - global[chest].column( 3 ).toVector3D();
        if ( reach.length() > farthest )
        {
            farthest = reach.length();
            moment = t;
        }
    }
    return moment;
}

/*-------------------------------------------------------------------*/
Figures3D::UpSchedule
Figures3D::upSchedule( const float duration,
                       const float ground,
                       const float end )
{
    UpSchedule s;
    const float room = end - ground; // cycles from the body down to the action's end
    if ( room <= 0.0f )
    {
        // no time: down to the end (the pose then goes over to walking)
        s.start = end;
        s.offset = duration;
        return s;
    }
    const float length = duration / CYCLE_TIME; // cycles at its own speed
    s.speed = std::min( UP_SPEED_MAX, std::max( 1.0f, length / room ) );
    const float played = length / s.speed;
    if ( played <= room )
    {
        s.start = end - played;
    }
    else
    {
        // its start left out
        s.start = ground;
        s.offset = duration - s.speed * CYCLE_TIME * room;
    }
    return s;
}

/*-------------------------------------------------------------------*/
float
Figures3D::diveOf( const FigureActions & actions ) const
{
    if ( ! actions.caught.on || ! motion().actions.dive.valid() ) return 0.0f;
    return clamp01( ( std::fabs( actions.caught.side ) - DIVE_FROM ) / ( DIVE_FULL - DIVE_FROM ) );
}

/*-------------------------------------------------------------------*/
HumanoidPose
Figures3D::rise( const HumanoidPose & down,
                 const ActionClip & up,
                 const float phase,
                 const float ground,
                 const float end ) const
{
    std::vector< NodeTransform > nodes;
    HumanoidPose pose = down;
    if ( up.valid() )
    {
        const UpSchedule s = upSchedule( up.duration, ground, end );
        if ( phase > s.start )
        {
            const HumanoidPose rise = sample( up.clip, up.at( s.offset + s.speed * CYCLE_TIME * ( phase - s.start ) ), nodes );
            const float blend = std::max( 1.0e-3f, std::min( UP_BLEND, 0.5f * ( end - s.start ) ) );
            pose = HumanoidPose::mix( pose, rise, curve( { { 0.0f, 0.0f }, { blend, 1.0f } }, phase - s.start ) );
        }
    }
    return pose;
}

/*-------------------------------------------------------------------*/
void
Figures3D::setLeg( const Body & body,
                   HumanoidPose & pose,
                   const int right,
                   const LimbPose & limb )
{
    // A field angle (to the right) is the model's turned the other way
    // about the vertical; swinging forward turns -y toward +z, about -x.
    const QQuaternion & down = body.leg_down[right];
    const QQuaternion swing = aboutY( -limb.yaw ) * aboutX( -limb.pitch )
        * aboutY( right ? -limb.turn : limb.turn ) * down;
    const HumanBone upper = ( right ? HumanBone::RightUpperLeg : HumanBone::LeftUpperLeg );
    const HumanBone lower = ( right ? HumanBone::RightLowerLeg : HumanBone::LeftLowerLeg );
    // in the model's directions, not the hips' (a clip may lean them): a
    // leg on the ground stays under the body
    body.rig.setTurnedInModel( pose, upper, swing );
    // the knee bends back: about +x from hanging down
    pose[lower] = ( down.conjugated() * aboutX( limb.bend ) * down ).normalized();
}

/*-------------------------------------------------------------------*/
void
Figures3D::setArm( const Body & body,
                   HumanoidPose & pose,
                   const int right,
                   const LimbPose & limb,
                   const bool in_model )
{
    const QQuaternion & down = body.arm_down[right];
    const QQuaternion swing = aboutY( -limb.yaw ) * aboutX( -limb.pitch )
        * aboutY( right ? -limb.turn : limb.turn ) * down;
    const HumanBone upper = ( right ? HumanBone::RightUpperArm : HumanBone::LeftUpperArm );
    const HumanBone lower = ( right ? HumanBone::RightLowerArm : HumanBone::LeftLowerArm );
    const HumanBone chest = ( body.rig.node( HumanBone::UpperChest ) >= 0 ? HumanBone::UpperChest : HumanBone::Chest );
    body.rig.setTurnedInModel( pose, upper, in_model ? swing : body.rig.turnedInModel( pose, chest ) * swing );
    // the elbow bends forward: about -x from hanging down
    pose[lower] = ( down.conjugated() * aboutX( -limb.bend ) * down ).normalized();
}

/*-------------------------------------------------------------------*/
void
Figures3D::act( const Body & b,
                const FigureInstance & figure,
                HumanoidPose & pose ) const
{
    const FigureActions & actions = figure.actions;

    // the head by the neck, spread over the chest, the neck and the head
    if ( actions.neck != 0.0f )
    {
        const float n = actions.neck;
        const bool neck = ( b.rig.node( HumanBone::Neck ) >= 0 );
        b.rig.turnInModel( pose, HumanBone::Chest, aboutY( -NECK_CHEST * n ) );
        if ( neck ) b.rig.turnInModel( pose, HumanBone::Neck, aboutY( -NECK_NECK * n ) );
        b.rig.turnInModel( pose, HumanBone::Head, aboutY( -( neck ? NECK_HEAD : NECK_NECK + NECK_HEAD ) * n ) );
    }

    const Weights w = weightsOf( actions );

    // the hips down by part of their height, the upper body leaned forward (+)
    auto body = [&]( HumanoidPose & target,
                     const float drop,
                     const float lean )
        {
            target.hips.setY( target.hips.y() - drop );
            b.rig.turnInModel( target, HumanBone::Spine, aboutX( 0.5f * lean ) );
            b.rig.turnInModel( target, HumanBone::Chest, aboutX( 0.5f * lean ) );
        };

    // the body: one action at a time, the fall after a foul first; by the
    // clips of the pack's motion if told, or else the poses made here
    const ActionClips & clips = motion().actions;
    std::vector< NodeTransform > nodes; // an action clip's nodes (the bones without a role: the walk's)
    if ( w.fouled > 0.0f )
    {
        // sitting on the ground, leaning back on the hands (or falling by
        // the clip), from when the tackler has slid in; getting up by the
        // clip to end with the action (the player goes then)
        const FigureRun & f = actions.fouled;
        HumanoidPose target = pose;
        float ground = FigureActionLog::FOULED_START + FigureActionLog::FOULED_FALL;
        if ( clips.fall.valid() )
        {
            target = sample( clips.fall.clip, clips.fall.at( CYCLE_TIME * ( f.phase - FigureActionLog::FOULED_START ) ), nodes );
            ground = FigureActionLog::FOULED_START + clips.fall.moment / CYCLE_TIME;
        }
        else
        {
            body( target, 0.80f, -70.0f );
            setLeg( b, target, 0, { 88.0f, -8.0f, 15.0f } );
            setLeg( b, target, 1, { 90.0f, 8.0f, 20.0f } );
            setArm( b, target, 0, { -50.0f, -25.0f, 10.0f } );
            setArm( b, target, 1, { -50.0f, 25.0f, 10.0f } );
        }
        target = rise( target, clips.get_up, f.phase, ground, f.last + FigureActionLog::FOULED_TAIL );
        pose = HumanoidPose::mix( pose, target, w.fouled );
    }
    else if ( w.tackle > 0.0f )
    {
        // sliding with the right leg ahead (or by the clip, on the ground at
        // the moment); getting up by the clip to end with the action
        const FigureRun & t = actions.tackle;
        const ActionClip & tackle = clips.tackle;
        HumanoidPose target = pose;
        float ground = 0.0f;
        if ( tackle.valid() )
        {
            target = sample( tackle.clip, tackle.at( tackle.moment + CYCLE_TIME * t.phase ), nodes );
            ground = std::max( 0.0f, ( tackle.down - tackle.moment ) / CYCLE_TIME );
        }
        else
        {
            body( target, 0.45f, -35.0f );
            setLeg( b, target, 1, { 80.0f, 0.0f, 5.0f } );
            setLeg( b, target, 0, { 25.0f, 0.0f, 110.0f } );
            setArm( b, target, 0, { -40.0f, -20.0f, 20.0f } );
            setArm( b, target, 1, { -40.0f, 20.0f, 20.0f } );
        }
        target = rise( target, clips.tackle_up.valid() ? clips.tackle_up : clips.get_up,
                       t.phase, ground, t.last + FigureActionLog::TACKLE_TAIL );
        pose = HumanoidPose::mix( pose, target, w.tackle );
    }
    else if ( w.kick > 0.0f )
    {
        const FigureKick & kick = actions.kick;
        const ActionClip & clip = clips.kick[static_cast< int >( kick.kind )];
        if ( clip.valid() )
        {
            // the foot meeting the ball at the moment (the clip of the other foot mirrored)
            const HumanoidPose target = sample( kick.right ? clip.clip : clip.other,
                                                clip.at( clip.moment + CYCLE_TIME * kick.phase ), nodes );
            pose = HumanoidPose::mix( pose, target, w.kick );
        }
        else
        {
            // the leg on the ball's side swings back, through the ball at the
            // moment, and on; the other arm out for balance
            const float k = kick.phase;
            const int leg = ( kick.right ? 1 : 0 );
            const float out = ( leg ? -1.0f : 1.0f ); // the other arm's outward, to its side
            HumanoidPose target = pose;
            LimbPose swing;
            LimbPose arm;
            switch ( kick.kind ) {
            case KickKind::Shot:
                swing.pitch = curve( { { -1.0f, 0.0f }, { -0.45f, -50.0f }, { 0.0f, 25.0f }, { 0.6f, 85.0f }, { 1.5f, 0.0f } }, k );
                swing.bend = curve( { { -1.0f, 0.0f }, { -0.45f, 100.0f }, { 0.0f, 10.0f }, { 0.6f, 20.0f }, { 1.5f, 0.0f } }, k );
                swing.yaw = clampTo( kick.way, 30.0f );
                arm = { 50.0f, 40.0f * out, 20.0f };
                break;
            case KickKind::Pass:
                // with the inside of the foot: the leg turned outward
                swing.pitch = curve( { { -0.9f, 0.0f }, { -0.4f, -30.0f }, { 0.0f, 20.0f }, { 0.5f, 50.0f }, { 1.3f, 0.0f } }, k );
                swing.bend = curve( { { -0.9f, 0.0f }, { -0.4f, 70.0f }, { 0.0f, 15.0f }, { 0.5f, 15.0f }, { 1.3f, 0.0f } }, k );
                swing.yaw = clampTo( kick.way, 45.0f );
                swing.turn = 30.0f;
                arm = { 35.0f, 30.0f * out, 20.0f };
                break;
            case KickKind::Tap:
            default:
                swing.pitch = curve( { { -0.6f, 0.0f }, { -0.3f, -10.0f }, { 0.0f, 20.0f }, { 0.3f, 25.0f }, { 0.8f, 0.0f } }, k );
                swing.bend = curve( { { -0.6f, 0.0f }, { -0.3f, 40.0f }, { 0.0f, 15.0f }, { 0.8f, 0.0f } }, k );
                swing.yaw = clampTo( kick.way, 45.0f );
                break;
            }
            setLeg( b, target, leg, swing );
            setLeg( b, target, 1 - leg, { -5.0f, 0.0f, 15.0f } );
            if ( kick.kind != KickKind::Tap ) setArm( b, target, 1 - leg, arm );
            pose = HumanoidPose::mix( pose, target, w.kick );
        }
    }

    // the arms: toward the ball for the goalie's catch, bending down for a
    // low one, up for a high one (by the clips of a catch and a dive if
    // told); or the right one pointing
    if ( w.caught > 0.0f && w.fouled <= 0.0f )
    {
        const FigureCatch & c = actions.caught;
        const float high = clamp01( ( c.height - CATCH_LOW ) / ( CATCH_HIGH - CATCH_LOW ) );
        HumanoidPose target = pose;
        if ( ! clips.catch_low.valid() && ! clips.catch_high.valid() && ! clips.dive.valid() )
        {
            catchPose( b, actions, high, target );
        }
        else
        {
            // a height by its clip (the body turned toward the ball), or else the pose made here
            const auto at = [&]( const ActionClip & a,
                                 const float h ) {
                HumanoidPose p = pose;
                if ( a.valid() )
                {
                    p = sample( a.clip, a.at( a.moment + CYCLE_TIME * c.phase ), nodes );
                    b.rig.turnInModel( p, HumanBone::Hips, aboutY( -clampTo( c.way, CATCH_TURN ) ) );
                }
                else
                {
                    catchPose( b, actions, h, p );
                }
                return p;
            };
            target = ( high <= 0.0f ? at( clips.catch_low, 0.0f )
                       : high >= 1.0f ? at( clips.catch_high, 1.0f )
                       : HumanoidPose::mix( at( clips.catch_low, 0.0f ), at( clips.catch_high, 1.0f ), high ) );
            // a ball to the side by a dive, down at its moment, getting up to end with the catch
            const float dive = diveOf( actions );
            if ( dive > 0.0f )
            {
                const ActionClip & d = clips.dive;
                const HumanoidPose diving = rise( sample( c.side >= 0.0f ? d.clip : d.other, d.at( d.moment + CYCLE_TIME * c.phase ), nodes ),
                                                  clips.get_up, c.phase,
                                                  std::max( 0.0f, ( d.down - d.moment ) / CYCLE_TIME ),
                                                  c.hold + FigureActionLog::CATCH_TAIL );
                target = HumanoidPose::mix( target, diving, dive );
            }
        }
        pose = HumanoidPose::mix( pose, target, w.caught );
    }
    else if ( actions.pointing
              && w.kick <= 0.0f && w.tackle <= 0.0f && w.fouled <= 0.0f )
    {
        // with the arm on the side it points to; far to the side or behind,
        // the chest turns toward it too (the head keeping its way, by the
        // neck); the arm lower for a point near the feet, the elbow a little bent
        const float way = actions.point_way;
        const int arm = ( way >= 0.0f ? 1 : 0 );
        const float turn = ( way >= 0.0f ? 1.0f : -1.0f )
            * std::min( POINT_TURN_MAX, std::max( 0.0f, std::fabs( way ) - POINT_TURN_FROM ) * 0.5f );
        if ( turn != 0.0f )
        {
            b.rig.turnInModel( pose, HumanBone::Chest, aboutY( -turn ) );
            b.rig.turnInModel( pose,
                               b.rig.node( HumanBone::Neck ) >= 0 ? HumanBone::Neck : HumanBone::Head,
                               aboutY( turn ) );
        }
        const float pitch = std::max( POINT_PITCH_MIN,
                                      std::atan2( actions.point_distance, POINT_SHOULDER ) * 180.0f / float( M_PI ) );
        setArm( b, pose, arm, { pitch, way, 10.0f }, true );
    }
}

/*-------------------------------------------------------------------*/
void
Figures3D::catchPose( const Body & b,
                      const FigureActions & actions,
                      const float high,
                      HumanoidPose & target ) const
{
    const float yaw = clampTo( actions.caught.way, CATCH_TURN );
    const float low = 1.0f - high;
    // low: crouching deep (the feet under the hips, which go down as
    // much), leaning far forward, the hands down near the ground
    // bending at the hips too (the legs are set after it, in the model's directions)
    b.rig.turnInModel( target, HumanBone::Hips, aboutX( 40.0f * low ) );
    // the hips down by part of their height, the upper body leaned forward
    const float lean = 45.0f * low + 5.0f * high;
    target.hips.setY( target.hips.y() - 0.35f * low );
    b.rig.turnInModel( target, HumanBone::Spine, aboutX( 0.5f * lean ) );
    b.rig.turnInModel( target, HumanBone::Chest, aboutX( 0.5f * lean ) );
    if ( low > 0.0f )
    {
        setLeg( b, target, 0, { 60.0f * low, 0.0f, 110.0f * low } );
        setLeg( b, target, 1, { 60.0f * low, 0.0f, 110.0f * low } );
    }
    // the arms' swing is from the chest, leaned with it: 85 from the
    // chest leaned 85 is hanging straight down to the ground
    const LimbPose arm{ 85.0f + ( 150.0f - 85.0f ) * high, yaw, 10.0f };
    setArm( b, target, 0, arm );
    setArm( b, target, 1, arm );
}

/*-------------------------------------------------------------------*/
float
Figures3D::reachOf( const FigureActions & actions )
{
    if ( ! actions.kick.on ) return 0.0f;
    const Weights w = weightsOf( actions );
    if ( w.fouled > 0.0f || w.tackle > 0.0f ) return 0.0f; // not kicking then
    // all the way at the moment, even for a touch (whose pose keeps some running)
    const float top = ( actions.kick.kind == KickKind::Tap ? KICK_TAP_WEIGHT : 1.0f );
    return clamp01( w.kick / top )
        * curve( { { -REACH_BEFORE, 0.0f }, { 0.0f, 1.0f }, { REACH_AFTER, 0.0f } }, actions.kick.phase );
}

/*-------------------------------------------------------------------*/
QVector3D
Figures3D::kickTarget( const FigureKick & kick )
{
    // the back of the ball at its center's height, from where the foot sends
    // it; the world's x and z are the field's x and y
    const float r = kick.ball_radius;
    return QVector3D( kick.ball_x - kick.dir_x * r, r, kick.ball_y - kick.dir_y * r );
}

/*-------------------------------------------------------------------*/
void
Figures3D::reach( const Body & body,
                  const FigureInstance & figure,
                  HumanoidPose & pose,
                  const std::vector< NodeTransform > & nodes ) const
{
    const float w = reachOf( figure.actions );
    if ( w <= 0.0f ) return;
    const bool right = figure.actions.kick.right;
    const HumanBone upper_bone = ( right ? HumanBone::RightUpperLeg : HumanBone::LeftUpperLeg );
    const HumanBone lower_bone = ( right ? HumanBone::RightLowerLeg : HumanBone::LeftLowerLeg );
    const HumanBone foot_bone = ( right ? HumanBone::RightFoot : HumanBone::LeftFoot );
    const HumanBone toes_bone = ( right ? HumanBone::RightToes : HumanBone::LeftToes );

    // where the leg is in the model in the pose so far
    std::vector< NodeTransform > local = nodes;
    body.rig.apply( pose, local );
    std::vector< QMatrix4x4 > global;
    body.model.globals( local, global );
    auto at = [&]( const HumanBone b ) { return global[body.rig.node( b )].column( 3 ).toVector3D(); };
    const QVector3D hip = at( upper_bone );
    const QVector3D knee = at( lower_bone );
    const QVector3D ankle = at( foot_bone );
    const QVector3D toes = ( body.rig.node( toes_bone ) >= 0 ? at( toes_bone ) : ankle );

    // the toes toward the ball, by the weight; the foot keeps its turn
    const QVector3D ball = modelMatrix( body.model, figure ).inverted().map( kickTarget( figure.actions.kick ) );
    const QVector3D toes_goal = toes + ( ball - toes ) * w;
    const QVector3D ankle_goal = toes_goal - ( toes - ankle );

    // the thigh and the shin (two bones) reaching the ankle's goal, the knee
    // bending forward (the model's +z), or as now if the leg points that way
    const float a = ( knee - hip ).length();
    const float b = ( ankle - knee ).length();
    const QVector3D to = ankle_goal - hip;
    if ( a < 1.0e-4f || b < 1.0e-4f || to.length() < 1.0e-4f ) return;
    const float d = std::min( a + b - 1.0e-4f, std::max( std::fabs( a - b ) + 1.0e-4f, to.length() ) );
    const QVector3D dir = to.normalized();
    const QVector3D forward( 0.0f, 0.0f, 1.0f );
    QVector3D side = forward - dir * QVector3D::dotProduct( forward, dir );
    if ( side.length() < 0.1f ) side = ( knee - hip ) - dir * QVector3D::dotProduct( knee - hip, dir );
    if ( side.length() < 1.0e-4f ) return;
    side.normalize();
    const float cos_hip = std::min( 1.0f, std::max( -1.0f, ( a * a + d * d - b * b ) / ( 2.0f * a * d ) ) );
    const QVector3D knee_goal = hip + dir * ( a * cos_hip ) + side * ( a * std::sqrt( 1.0f - cos_hip * cos_hip ) );

    const QQuaternion thigh = QQuaternion::rotationTo( knee - hip, knee_goal - hip );
    body.rig.turnInModel( pose, upper_bone, thigh );
    const QVector3D ankle_now = hip + thigh.rotatedVector( ankle - hip );
    const QQuaternion shin = QQuaternion::rotationTo( ankle_now - knee_goal, hip + dir * d - knee_goal );
    body.rig.turnInModel( pose, lower_bone, shin );
    body.rig.turnInModel( pose, foot_bone, ( shin * thigh ).conjugated() );
}

/*-------------------------------------------------------------------*/
HumanoidPose
Figures3D::sample( const int clip,
                   const float time,
                   std::vector< NodeTransform > & motion ) const
{
    const Motion & m = this->motion();
    motion = m.model.rest;
    m.model.sample( m.model.clips[clip], time, motion );
    return m.rig.poseOf( motion );
}

/*-------------------------------------------------------------------*/
void
Figures3D::localPose( const FigureInstance & figure,
                      const double time,
                      std::vector< NodeTransform > & local ) const
{
    local = model( bodyOf( figure ) ).rest;
    if ( bodyCount() == 0 ) return;
    const Body & body = bodyFor( figure );

    const FigureMotion & motion = figure.motion;
    const Gait gait = gaitOf( figure );

    // standing: the idle clip at the game's time, each player at its own
    // point of it
    HumanoidPose pose;
    std::vector< NodeTransform > nodes;
    if ( gait.moving < 1.0f )
    {
        pose = sample( this->motion().idle, static_cast< float >( time + figure.player * IDLE_OFFSET ), nodes );
    }

    // walking and running: the gait clips at the phase of the gait walked
    // for the figure's height (a taller one takes longer strides, and so
    // does a body of longer legs), the left foot coming down at its start
    if ( gait.moving > 0.0f )
    {
        const double cycles = motion.gait * SceneBuilder::PERSON_HEIGHT
            / ( std::max( 0.1f, figure.height ) * body.stride );
        const float phase = static_cast< float >( cycles - std::floor( cycles ) );
        // each way's clips mixed by the speed, then the two ways; the
        // bones without a role (the fingers) of the clip that counts most
        float most = -1.0f;
        std::vector< NodeTransform > clip_nodes;
        const auto wayPose = [&]( const int way,
                                  const Mix & mix,
                                  const float weight ) {
            const std::vector< GaitClip > & clips = wayClips( way );
            const GaitClip & a = clips[mix.from];
            const GaitClip & b = clips[mix.to];
            HumanoidPose p = sample( a.clip, a.timeAt( phase ), clip_nodes );
            if ( weight * ( 1.0f - mix.t ) > most )
            {
                most = weight * ( 1.0f - mix.t );
                if ( gait.moving >= 0.5f ) nodes = clip_nodes;
            }
            if ( mix.to != mix.from && mix.t > 0.0f )
            {
                p = HumanoidPose::mix( p, sample( b.clip, b.timeAt( phase ), clip_nodes ), mix.t );
                if ( weight * mix.t > most )
                {
                    most = weight * mix.t;
                    if ( gait.moving >= 0.5f ) nodes = clip_nodes;
                }
            }
            return p;
        };
        HumanoidPose moving = wayPose( gait.ways.a, gait.a, 1.0f - gait.ways.t );
        if ( gait.ways.b != gait.ways.a && gait.ways.t > 0.0f )
        {
            moving = HumanoidPose::mix( moving, wayPose( gait.ways.b, gait.b, gait.ways.t ), gait.ways.t );
        }
        pose = ( gait.moving < 1.0f ? HumanoidPose::mix( pose, moving, gait.moving ) : moving );

        // the hips toward the way the figure goes, the chest keeping the
        // body direction. A field angle is the model's turned the other
        // way about the vertical.
        const float twist = twistOf( figure );
        if ( twist != 0.0f )
        {
            body.rig.turnInModel( pose, HumanBone::Hips, QQuaternion::fromAxisAndAngle( 0.0f, 1.0f, 0.0f, -twist ) );
            body.rig.turnInModel( pose, HumanBone::Spine, QQuaternion::fromAxisAndAngle( 0.0f, 1.0f, 0.0f, 0.5f * twist ) );
            body.rig.turnInModel( pose, HumanBone::Chest, QQuaternion::fromAxisAndAngle( 0.0f, 1.0f, 0.0f, 0.5f * twist ) );
        }
    }

    // the neck and the actions over the clips, and the kicking foot to the
    // ball, on the body drawn
    bodyNodes( body, nodes, local );
    act( body, figure, pose );
    reach( body, figure, pose, local );

    // through the humanoid layer: the bones without a role as the motion's
    body.rig.apply( pose, local );
}

/*-------------------------------------------------------------------*/
void
Figures3D::bodyNodes( const Body & body,
                      const std::vector< NodeTransform > & motion,
                      std::vector< NodeTransform > & local ) const
{
    const GltfModel & from = motionModel();
    if ( body.motion_skeleton
         || motion.size() != from.rest.size() )
    {
        local = ( body.motion_skeleton ? motion : body.model.rest );
        return;
    }
    // each bone turned from the body's rest as the motion's from its rest;
    // the bones of the roles are put by the humanoid layer after it
    local = body.model.rest;
    for ( std::size_t i = 0; i < local.size(); ++i )
    {
        const int m = body.motion_node[i];
        if ( m < 0 ) continue;
        local[i].r = ( local[i].r * from.rest[m].r.conjugated() * motion[m].r ).normalized();
    }
}

/*-------------------------------------------------------------------*/
void
Figures3D::worldNodes( const FigureInstance & figure,
                       const double time,
                       std::vector< QMatrix4x4 > & world ) const
{
    world.clear();
    if ( bodyCount() == 0 ) return;
    std::vector< NodeTransform > local;
    localPose( figure, time, local );
    model( bodyOf( figure ) ).globals( local, world );
    const QMatrix4x4 place = modelMatrix( figure );
    for ( QMatrix4x4 & m : world ) m = place * m;
}

/*-------------------------------------------------------------------*/
void
Figures3D::pose( SceneContent & content,
                 const double time ) const
{
    content.figure_joints.clear();
    content.figure_bodies.clear();
    if ( bodyCount() == 0 ) return;

    // each figure's joints after the one before's (the bodies of one skeleton: as many joints)
    std::size_t offset = 0;
    for ( const FigureInstance & figure : content.figures )
    {
        offset += static_cast< std::size_t >( model( bodyOf( figure ) ).jointCount() ) * 16;
    }
    content.figure_joints.resize( offset );
    offset = 0;
    std::vector< NodeTransform > local;
    for ( const FigureInstance & figure : content.figures )
    {
        const int body = bodyOf( figure );
        const GltfModel & m = model( body );
        localPose( figure, time, local );
        m.skin( local, modelMatrix( m, figure ), &content.figure_joints[offset] );
        offset += static_cast< std::size_t >( m.jointCount() ) * 16;
        content.figure_bodies.push_back( body );
    }
}
