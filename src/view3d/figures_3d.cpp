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
    // each limb from its rest to hanging down
    {
        std::vector< QMatrix4x4 > global;
        M_model.globals( M_model.rest, global );
        auto down = [&]( const HumanBone upper,
                         const HumanBone lower )
            {
                if ( M_rig.node( upper ) < 0 || M_rig.node( lower ) < 0 ) return QQuaternion();
                const QVector3D dir = ( global[M_rig.node( lower )].column( 3 ) - global[M_rig.node( upper )].column( 3 ) ).toVector3D();
                return QQuaternion::rotationTo( dir.normalized(), QVector3D( 0.0f, -1.0f, 0.0f ) );
            };
        M_leg_down[0] = down( HumanBone::LeftUpperLeg, HumanBone::LeftLowerLeg );
        M_leg_down[1] = down( HumanBone::RightUpperLeg, HumanBone::RightLowerLeg );
        M_arm_down[0] = down( HumanBone::LeftUpperArm, HumanBone::LeftLowerArm );
        M_arm_down[1] = down( HumanBone::RightUpperArm, HumanBone::RightLowerArm );
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
    // to a ball to catch to the side: +y turned toward the right, -x
    const float roll = rollOf( figure );
    if ( roll != 0.0f )
    {
        m.rotate( roll, 0.0f, 0.0f, 1.0f );
    }

    m.scale( figure.height / std::max( 0.01f, M_model.height() ) );
    m.translate( 0.0f, -M_model.min_y, 0.0f ); // the feet on the ground
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
    return weightsOf( figure.actions ).caught * reach * CATCH_ROLL * ( c.side < 0.0f ? -1.0f : 1.0f );
}

/*-------------------------------------------------------------------*/
void
Figures3D::setLeg( HumanoidPose & pose,
                   const int right,
                   const LimbPose & limb ) const
{
    // A field angle (to the right) is the model's turned the other way
    // about the vertical; swinging forward turns -y toward +z, about -x.
    const QQuaternion & down = M_leg_down[right];
    const QQuaternion swing = aboutY( -limb.yaw ) * aboutX( -limb.pitch )
        * aboutY( right ? -limb.turn : limb.turn ) * down;
    const HumanBone upper = ( right ? HumanBone::RightUpperLeg : HumanBone::LeftUpperLeg );
    const HumanBone lower = ( right ? HumanBone::RightLowerLeg : HumanBone::LeftLowerLeg );
    // in the model's directions, not the hips' (a clip may lean them): a
    // leg on the ground stays under the body
    M_rig.setTurnedInModel( pose, upper, swing );
    // the knee bends back: about +x from hanging down
    pose[lower] = ( down.conjugated() * aboutX( limb.bend ) * down ).normalized();
}

/*-------------------------------------------------------------------*/
void
Figures3D::setArm( HumanoidPose & pose,
                   const int right,
                   const LimbPose & limb,
                   const bool in_model ) const
{
    const QQuaternion & down = M_arm_down[right];
    const QQuaternion swing = aboutY( -limb.yaw ) * aboutX( -limb.pitch )
        * aboutY( right ? -limb.turn : limb.turn ) * down;
    const HumanBone upper = ( right ? HumanBone::RightUpperArm : HumanBone::LeftUpperArm );
    const HumanBone lower = ( right ? HumanBone::RightLowerArm : HumanBone::LeftLowerArm );
    const HumanBone chest = ( M_rig.node( HumanBone::UpperChest ) >= 0 ? HumanBone::UpperChest : HumanBone::Chest );
    M_rig.setTurnedInModel( pose, upper, in_model ? swing : M_rig.turnedInModel( pose, chest ) * swing );
    // the elbow bends forward: about -x from hanging down
    pose[lower] = ( down.conjugated() * aboutX( -limb.bend ) * down ).normalized();
}

/*-------------------------------------------------------------------*/
void
Figures3D::act( const FigureInstance & figure,
                HumanoidPose & pose ) const
{
    const FigureActions & actions = figure.actions;

    // the head by the neck, spread over the chest, the neck and the head
    if ( actions.neck != 0.0f )
    {
        const float n = actions.neck;
        const bool neck = ( M_rig.node( HumanBone::Neck ) >= 0 );
        M_rig.turnInModel( pose, HumanBone::Chest, aboutY( -NECK_CHEST * n ) );
        if ( neck ) M_rig.turnInModel( pose, HumanBone::Neck, aboutY( -NECK_NECK * n ) );
        M_rig.turnInModel( pose, HumanBone::Head, aboutY( -( neck ? NECK_HEAD : NECK_NECK + NECK_HEAD ) * n ) );
    }

    const Weights w = weightsOf( actions );

    // the hips down by part of their height, the upper body leaned forward (+)
    auto body = [&]( HumanoidPose & target,
                     const float drop,
                     const float lean )
        {
            target.hips.setY( target.hips.y() - drop );
            M_rig.turnInModel( target, HumanBone::Spine, aboutX( 0.5f * lean ) );
            M_rig.turnInModel( target, HumanBone::Chest, aboutX( 0.5f * lean ) );
        };

    // the body: one action at a time, the fall after a foul first
    if ( w.fouled > 0.0f )
    {
        // sitting on the ground, leaning back on the hands
        HumanoidPose target = pose;
        body( target, 0.80f, -70.0f );
        setLeg( target, 0, { 88.0f, -8.0f, 15.0f } );
        setLeg( target, 1, { 90.0f, 8.0f, 20.0f } );
        setArm( target, 0, { -50.0f, -25.0f, 10.0f } );
        setArm( target, 1, { -50.0f, 25.0f, 10.0f } );
        pose = HumanoidPose::mix( pose, target, w.fouled );
    }
    else if ( w.tackle > 0.0f )
    {
        // sliding with the right leg ahead
        HumanoidPose target = pose;
        body( target, 0.45f, -35.0f );
        setLeg( target, 1, { 80.0f, 0.0f, 5.0f } );
        setLeg( target, 0, { 25.0f, 0.0f, 110.0f } );
        setArm( target, 0, { -40.0f, -20.0f, 20.0f } );
        setArm( target, 1, { -40.0f, 20.0f, 20.0f } );
        pose = HumanoidPose::mix( pose, target, w.tackle );
    }
    else if ( w.kick > 0.0f )
    {
        // the leg on the ball's side swings back, through the ball at the
        // moment, and on; the other arm out for balance
        const FigureKick & kick = actions.kick;
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
        setLeg( target, leg, swing );
        setLeg( target, 1 - leg, { -5.0f, 0.0f, 15.0f } );
        if ( kick.kind != KickKind::Tap ) setArm( target, 1 - leg, arm );
        pose = HumanoidPose::mix( pose, target, w.kick );
    }

    // the arms: toward the ball for the goalie's catch, bending down for a
    // low one, up for a high one; or the right one pointing
    if ( w.caught > 0.0f && w.fouled <= 0.0f )
    {
        const float yaw = clampTo( actions.caught.way, 50.0f );
        const float high = clamp01( ( actions.caught.height - CATCH_LOW ) / ( CATCH_HIGH - CATCH_LOW ) );
        HumanoidPose target = pose;
        const float low = 1.0f - high;
        // low: crouching deep (the feet under the hips, which go down as
        // much), leaning far forward, the hands down near the ground
        // bending at the hips too (the legs are set after it, in the model's directions)
        M_rig.turnInModel( target, HumanBone::Hips, aboutX( 40.0f * low ) );
        body( target, 0.35f * low, 45.0f * low + 5.0f * high );
        if ( low > 0.0f )
        {
            setLeg( target, 0, { 60.0f * low, 0.0f, 110.0f * low } );
            setLeg( target, 1, { 60.0f * low, 0.0f, 110.0f * low } );
        }
        // the arms' swing is from the chest, leaned with it: 85 from the
        // chest leaned 85 is hanging straight down to the ground
        const LimbPose arm{ 85.0f + ( 150.0f - 85.0f ) * high, yaw, 10.0f };
        setArm( target, 0, arm );
        setArm( target, 1, arm );
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
            M_rig.turnInModel( pose, HumanBone::Chest, aboutY( -turn ) );
            M_rig.turnInModel( pose,
                               M_rig.node( HumanBone::Neck ) >= 0 ? HumanBone::Neck : HumanBone::Head,
                               aboutY( turn ) );
        }
        const float pitch = std::max( POINT_PITCH_MIN,
                                      std::atan2( actions.point_distance, POINT_SHOULDER ) * 180.0f / float( M_PI ) );
        setArm( pose, arm, { pitch, way, 10.0f }, true );
    }
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
Figures3D::reach( const FigureInstance & figure,
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
    M_rig.apply( pose, local );
    std::vector< QMatrix4x4 > global;
    M_model.globals( local, global );
    auto at = [&]( const HumanBone b ) { return global[M_rig.node( b )].column( 3 ).toVector3D(); };
    const QVector3D hip = at( upper_bone );
    const QVector3D knee = at( lower_bone );
    const QVector3D ankle = at( foot_bone );
    const QVector3D toes = ( M_rig.node( toes_bone ) >= 0 ? at( toes_bone ) : ankle );

    // the toes toward the ball, by the weight; the foot keeps its turn
    const QVector3D ball = modelMatrix( figure ).inverted().map( kickTarget( figure.actions.kick ) );
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
    M_rig.turnInModel( pose, upper_bone, thigh );
    const QVector3D ankle_now = hip + thigh.rotatedVector( ankle - hip );
    const QQuaternion shin = QQuaternion::rotationTo( ankle_now - knee_goal, hip + dir * d - knee_goal );
    M_rig.turnInModel( pose, lower_bone, shin );
    M_rig.turnInModel( pose, foot_bone, ( shin * thigh ).conjugated() );
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

    // the neck and the actions over the clips, and the kicking foot to the ball
    act( figure, pose );
    reach( figure, pose, nodes );

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
