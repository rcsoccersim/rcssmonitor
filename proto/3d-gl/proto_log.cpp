// -*-c++-*-

/*
  A prototype of the 3D view (docs/PLAN_3D-VIEW.md, Phase 0, D1). Not part
  of the application.
*/

#include "proto_log.h"

#include "game_log_file.h"

#include <rcss/rcg/handler.h>
#include <rcss/rcg/types.h>

#include <algorithm>
#include <cmath>
#include <iostream>

namespace {

class ShowCollector
    : public rcss::rcg::Handler {
public:
    std::vector< ProtoFrame > * frames_;

    bool handleEOF() override { return true; }
    bool handleShow( const rcss::rcg::ShowInfoT & show ) override
      {
          ProtoFrame f;
          f.time = static_cast< int >( show.time_ );
          f.ball_x = show.ball_.x_;
          f.ball_y = show.ball_.y_;
          for ( int i = 0; i < PROTO_PLAYERS; ++i )
          {
              const rcss::rcg::PlayerT & p = show.player_[i];
              ProtoPlayer & q = f.players[i];
              q.x = p.x_;
              q.y = p.y_;
              q.vx = ( p.hasVelocity() ? p.vx_ : 0.0f );
              q.vy = ( p.hasVelocity() ? p.vy_ : 0.0f );
              q.body = p.body_;
              q.neck = ( p.hasNeck() ? p.neck_ : 0.0f );
              q.state = p.state_;
          }
          frames_->push_back( f );
          return true;
      }
    bool handleMsg( const int, const int, const std::string & ) override { return true; }
    bool handleDraw( const int, const rcss::rcg::drawinfo_t & ) override { return true; }
    bool handlePlayMode( const int, const rcss::rcg::PlayMode ) override { return true; }
    bool handleTeam( const int, const rcss::rcg::TeamT &, const rcss::rcg::TeamT & ) override { return true; }
    bool handleServerParam( const rcss::rcg::ServerParamT & ) override { return true; }
    bool handlePlayerParam( const rcss::rcg::PlayerParamT & ) override { return true; }
    bool handlePlayerType( const rcss::rcg::PlayerTypeT & ) override { return true; }
    bool handleTeamGraphic( const char, const int, const int, const std::vector< std::string > & ) override { return true; }
};

}

/*-------------------------------------------------------------------*/
bool
ProtoLog::load( const std::string & path )
{
    frames_.clear();
    ShowCollector collector;
    collector.frames_ = &frames_;
    if ( ! loadGameLogFile( path, collector ) || frames_.empty() )
    {
        std::cerr << "could not read the shows of " << path << std::endl;
        return false;
    }

    computeGait();
    return true;
}

/*-------------------------------------------------------------------*/
float
ProtoLog::speed( const ProtoPlayer & p )
{
    return std::hypot( p.vx, p.vy ) / PROTO_STEP;
}

/*-------------------------------------------------------------------*/
void
ProtoLog::computeGait()
{
    distance_.assign( frames_.size() * PROTO_PLAYERS, 0.0f );
    for ( std::size_t f = 1; f < frames_.size(); ++f )
    {
        for ( int i = 0; i < PROTO_PLAYERS; ++i )
        {
            const ProtoPlayer & a = frames_[f - 1].players[i];
            const ProtoPlayer & b = frames_[f].players[i];
            const float dx = b.x - a.x;
            const float dy = b.y - a.y;
            float d = std::hypot( dx, dy );
            if ( d > 2.0f )
            {
                d = 0.0f; // a move, not walking
            }
            const float body = b.body * float( M_PI ) / 180.0f;
            if ( dx * std::cos( body ) + dy * std::sin( body ) < 0.0f )
            {
                d = -d; // backward
            }
            distance_[f * PROTO_PLAYERS + i] = distance_[( f - 1 ) * PROTO_PLAYERS + i] + d;
        }
    }
}

/*-------------------------------------------------------------------*/
namespace {

void
frames_at( double t,
           int n,
           int * f0,
           int * f1,
           float * u )
{
    const double w = std::fmod( std::fmod( t, double( n - 1 ) ) + ( n - 1 ), double( n - 1 ) );
    *f0 = static_cast< int >( w );
    *f1 = std::min( *f0 + 1, n - 1 );
    *u = static_cast< float >( w - *f0 );
}

float
lerp_angle( float a,
            float b,
            float u )
{
    const float d = std::fmod( b - a + 540.0f, 360.0f ) - 180.0f;
    return a + d * u;
}

}

/*-------------------------------------------------------------------*/
ProtoFigure
ProtoLog::figureAt( double t,
                    int i ) const
{
    int f0, f1;
    float u;
    frames_at( t, size(), &f0, &f1, &u );
    const ProtoPlayer & a = frames_[f0].players[i];
    const ProtoPlayer & b = frames_[f1].players[i];

    ProtoFigure fig;
    const bool jump = std::hypot( b.x - a.x, b.y - a.y ) > 2.0f;
    fig.x = ( jump ? a.x : a.x + ( b.x - a.x ) * u );
    fig.y = ( jump ? a.y : a.y + ( b.y - a.y ) * u );
    fig.body = lerp_angle( a.body, b.body, u );

    const float d0 = distance_[f0 * PROTO_PLAYERS + i];
    const float d1 = distance_[f1 * PROTO_PLAYERS + i];
    const float dist = d0 + ( d1 - d0 ) * u;
    const float v = speed( a );
    const auto ramp = []( float z ) { return std::min( 1.0f, std::max( 0.0f, z ) ); };
    const float sprint = ramp( ( v - 5.0f ) / 1.5f );
    const float run = ramp( ( v - 2.0f ) / 1.5f ) * ( 1.0f - sprint );
    const float walk = ramp( v / 0.8f ) * ( 1.0f - run - sprint );
    fig.weights[0] = std::max( 0.0f, 1.0f - walk - run - sprint );
    fig.weights[1] = walk;
    fig.weights[2] = run;
    fig.weights[3] = sprint;
    fig.times[0] = float( ( f0 + u ) * PROTO_STEP + i * 0.53 );
    fig.times[1] = dist / 1.4f;
    fig.times[2] = dist / 3.6f;
    fig.times[3] = dist / 6.5f;
    return fig;
}

/*-------------------------------------------------------------------*/
void
ProtoLog::ballAt( double t,
                  float * x,
                  float * y ) const
{
    int f0, f1;
    float u;
    frames_at( t, size(), &f0, &f1, &u );
    *x = frames_[f0].ball_x + ( frames_[f1].ball_x - frames_[f0].ball_x ) * u;
    *y = frames_[f0].ball_y + ( frames_[f1].ball_y - frames_[f0].ball_y ) * u;
}
