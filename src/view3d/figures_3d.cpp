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

#include "scene_builder.h"

#include <QFile>

#include <algorithm>
#include <cmath>

namespace {

//! the bundled model in the resources (resources/models/view3d_models.qrc)
const char * MODEL_PATH = ":/models/Mannequin.glb";
//! its clip of standing
const char * IDLE_CLIP = "Idle_Loop";
//! its clips of walking and running
const char * GAIT_CLIPS[] = { "Walk_Loop", "Jog_Fwd_Loop", "Sprint_Loop" };

//! the points of a clip read by analyzeGait()
constexpr int GAIT_SAMPLES = 240;
//! a foot is down while it is this part of its highest above its lowest
constexpr float FOOT_DOWN = 0.15f;
//! a foot of a walk is lifted at least this part of the model's height
constexpr float FOOT_LIFT = 0.02f;
//! a foot is on the ground while it goes back at this part of its usual speed while low
constexpr double FOOT_BACK = 0.5;

float
clamp01( const float v )
{
    return std::min( 1.0f, std::max( 0.0f, v ) );
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

/*-------------------------------------------------------------------*/
bool
Figures3D::load()
{
    if ( M_loaded ) return M_ready;
    M_loaded = true;

    // the resources of a static library must be registered before use
    Q_INIT_RESOURCE( view3d_models );

    QFile file( QString::fromLatin1( MODEL_PATH ) );
    if ( ! file.open( QIODevice::ReadOnly ) )
    {
        M_error = QStringLiteral( "cannot open " ) + file.fileName();
        return false;
    }
    QString error;
    if ( ! M_model.load( file.readAll(), &error ) )
    {
        M_error = file.fileName() + QStringLiteral( ": " ) + error;
        return false;
    }
    if ( ! M_rig.build( M_model, quaterniusBoneNames(), &error ) )
    {
        M_error = file.fileName() + QStringLiteral( ": " ) + error;
        return false;
    }
    M_idle = M_model.clipIndex( QString::fromLatin1( IDLE_CLIP ) );
    if ( M_idle < 0 )
    {
        M_error = file.fileName() + QStringLiteral( ": no clip " ) + IDLE_CLIP;
        return false;
    }
    for ( const char * name : GAIT_CLIPS )
    {
        GaitClip gait;
        if ( analyzeGait( M_model, M_rig, M_model.clipIndex( QString::fromLatin1( name ) ), &gait ) )
        {
            M_gaits.push_back( gait );
        }
    }
    std::sort( M_gaits.begin(), M_gaits.end(),
               []( const GaitClip & a, const GaitClip & b ) { return a.speed < b.speed; } );
    M_ready = true;
    return true;
}

/*-------------------------------------------------------------------*/
bool
Figures3D::analyzeGait( const GltfModel & model,
                        const HumanoidRig & rig,
                        const int clip,
                        GaitClip * out )
{
    if ( clip < 0
         || clip >= static_cast< int >( model.clips.size() )
         || model.clips[clip].duration <= 0.0f
         || model.height() <= 0.0f )
    {
        return false;
    }
    const GltfModel::Clip & c = model.clips[clip];
    const HumanBone feet[2] = { HumanBone::LeftFoot, HumanBone::RightFoot };
    const HumanBone toes[2] = { HumanBone::LeftToes, HumanBone::RightToes };

    // the ankles and the toes over the cycle
    std::vector< float > ankle_y[2], toe_y[2], z[2];
    std::vector< NodeTransform > local;
    std::vector< QMatrix4x4 > global;
    for ( int k = 0; k < GAIT_SAMPLES; ++k )
    {
        local = model.rest;
        model.sample( c, c.duration * k / GAIT_SAMPLES, local );
        model.globals( local, global );
        for ( int s = 0; s < 2; ++s )
        {
            const QVector3D ankle = global[rig.node( feet[s] )].column( 3 ).toVector3D();
            const QVector3D toe = ( rig.node( toes[s] ) >= 0
                                    ? global[rig.node( toes[s] )].column( 3 ).toVector3D()
                                    : ankle );
            ankle_y[s].push_back( ankle.y() );
            toe_y[s].push_back( toe.y() );
            z[s].push_back( ( ankle.z() + toe.z() ) * 0.5f );
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
        if ( high < FOOT_LIFT * model.height() ) return false; // not lifted: not walking
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
                const double before = z[s][( k + GAIT_SAMPLES - 1 ) % GAIT_SAMPLES];
                const double after = z[s][( k + 1 ) % GAIT_SAMPLES];
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
    out->speed = speed_sum * 0.5f * SceneBuilder::PERSON_HEIGHT / model.height();
    return true;
}

/*-------------------------------------------------------------------*/
std::vector< GaitStride >
Figures3D::strides() const
{
    std::vector< GaitStride > strides;
    for ( const GaitClip & g : M_gaits )
    {
        strides.push_back( { g.speed, g.stride() } );
    }
    return strides;
}

/*-------------------------------------------------------------------*/
Figures3D::Mix
Figures3D::mixAt( const float speed ) const
{
    Mix mix;
    if ( M_gaits.empty() ) return mix;
    mix.moving = clamp01( speed / ( MOVING_FULL * M_gaits.front().speed ) );
    const int last = static_cast< int >( M_gaits.size() ) - 1;
    mix.from = mix.to = last; // faster than all: the fastest
    for ( int k = 0; k <= last; ++k )
    {
        if ( speed <= M_gaits[k].speed )
        {
            // at a clip's speed or below the slowest: that clip; else from the one before
            mix.to = k;
            mix.from = ( k == 0 || speed == M_gaits[k].speed ? k : k - 1 );
            if ( mix.from != mix.to )
            {
                mix.t = ( speed - M_gaits[k - 1].speed ) / ( M_gaits[k].speed - M_gaits[k - 1].speed );
            }
            break;
        }
    }
    return mix;
}

/*-------------------------------------------------------------------*/
float
Figures3D::twistOf( const FigureInstance & figure ) const
{
    const float speed = std::hypot( figure.motion.vx, figure.motion.vy );
    if ( speed < 1.0e-3f ) return 0.0f;
    const float toward = std::atan2( figure.motion.vy, figure.motion.vx ) - figure.body * float( M_PI ) / 180.0f;
    // toward 45 degrees from the body, the hips turn as far: the most;
    // toward 90 (sideways, where the gait turns between forward and
    // backward), not at all, so that they do not jump there
    return TWIST_MAX * std::sin( 2.0f * toward ) * mixAt( speed ).moving;
}

/*-------------------------------------------------------------------*/
QStringList
Figures3D::jerseyMaterials()
{
    // the Mannequin's body; its joints (M_Joints) keep their own color
    return QStringList( { QStringLiteral( "M_Main" ) } );
}

/*-------------------------------------------------------------------*/
QMatrix4x4
Figures3D::modelMatrix( const FigureInstance & figure ) const
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

    m.scale( figure.height / std::max( 0.01f, M_model.height() ) );
    m.translate( 0.0f, -M_model.min_y, 0.0f ); // the feet on the ground
    return m;
}

/*-------------------------------------------------------------------*/
HumanoidPose
Figures3D::sample( const int clip,
                   const float time,
                   std::vector< NodeTransform > & motion ) const
{
    motion = M_model.rest;
    M_model.sample( M_model.clips[clip], time, motion );
    return M_rig.poseOf( motion );
}

/*-------------------------------------------------------------------*/
void
Figures3D::localPose( const FigureInstance & figure,
                      const double time,
                      std::vector< NodeTransform > & local ) const
{
    local = M_model.rest;
    if ( ! M_ready ) return;

    const FigureMotion & motion = figure.motion;
    const Mix mix = mixAt( std::hypot( motion.vx, motion.vy ) );

    // standing: the idle clip at the game's time, each player at its own
    // point of it
    HumanoidPose pose;
    std::vector< NodeTransform > nodes;
    if ( mix.moving < 1.0f )
    {
        pose = sample( M_idle, static_cast< float >( time + figure.player * IDLE_OFFSET ), nodes );
    }

    // walking and running: the gait clips at the phase of the gait walked
    // for the figure's height (a taller one takes longer strides), the
    // left foot coming down at its start
    if ( mix.moving > 0.0f )
    {
        const double cycles = motion.gait * SceneBuilder::PERSON_HEIGHT / std::max( 0.1f, figure.height );
        const float phase = static_cast< float >( cycles - std::floor( cycles ) );
        const GaitClip & a = M_gaits[mix.from];
        const GaitClip & b = M_gaits[mix.to];
        std::vector< NodeTransform > nodes_a, nodes_b;
        HumanoidPose moving = sample( a.clip, a.timeAt( phase ), nodes_a );
        if ( mix.to != mix.from && mix.t > 0.0f )
        {
            moving = HumanoidPose::mix( moving,
                                        sample( b.clip, b.timeAt( phase ), nodes_b ),
                                        mix.t );
        }
        pose = ( mix.moving < 1.0f ? HumanoidPose::mix( pose, moving, mix.moving ) : moving );
        // the bones without a role (the fingers) of the clip that counts most
        if ( mix.moving >= 0.5f ) nodes.swap( mix.t > 0.5f && ! nodes_b.empty() ? nodes_b : nodes_a );

        // the hips toward the way the figure goes, the chest keeping the
        // body direction. A field angle is the model's turned the other
        // way about the vertical.
        const float twist = twistOf( figure );
        if ( twist != 0.0f )
        {
            M_rig.turnInModel( pose, HumanBone::Hips, QQuaternion::fromAxisAndAngle( 0.0f, 1.0f, 0.0f, -twist ) );
            M_rig.turnInModel( pose, HumanBone::Spine, QQuaternion::fromAxisAndAngle( 0.0f, 1.0f, 0.0f, 0.5f * twist ) );
            M_rig.turnInModel( pose, HumanBone::Chest, QQuaternion::fromAxisAndAngle( 0.0f, 1.0f, 0.0f, 0.5f * twist ) );
        }
    }

    // through the humanoid layer: the motion's skeleton is the model's, so
    // the bones without a role take the motion's too
    local = nodes;
    M_rig.apply( pose, local );
}

/*-------------------------------------------------------------------*/
void
Figures3D::pose( SceneContent & content,
                 const double time ) const
{
    content.figure_joints.clear();
    if ( ! M_ready ) return;

    const std::size_t floats = static_cast< std::size_t >( M_model.jointCount() ) * 16;
    content.figure_joints.resize( content.figures.size() * floats );
    std::vector< NodeTransform > local;
    for ( std::size_t k = 0; k < content.figures.size(); ++k )
    {
        const FigureInstance & figure = content.figures[k];
        localPose( figure, time, local );
        M_model.skin( local, modelMatrix( figure ), &content.figure_joints[k * floats] );
    }
}
