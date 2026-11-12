// -*-c++-*-

/*
  A prototype of the 3D view (docs/PLAN_3D-VIEW.md, Phase 0, D1): the
  frames of a game log, for the figures to walk by. Not part of the
  application.
*/

#ifndef PROTO3D_PROTO_LOG_H
#define PROTO3D_PROTO_LOG_H

#include <string>
#include <vector>

//! the number of the players
constexpr int PROTO_PLAYERS = 22;

//! the seconds of a cycle
constexpr float PROTO_STEP = 0.1f;

struct ProtoPlayer {
    float x = 0.0f; //!< position x (m)
    float y = 0.0f; //!< position y (m)
    float vx = 0.0f; //!< velocity x (m/cycle)
    float vy = 0.0f; //!< velocity y (m/cycle)
    float body = 0.0f; //!< body direction (degree)
    float neck = 0.0f; //!< neck direction relative to the body (degree)
    int state = 0; //!< state bits, 0 if disabled
};

struct ProtoFrame {
    int time = 0;
    float ball_x = 0.0f;
    float ball_y = 0.0f;
    ProtoPlayer players[PROTO_PLAYERS]; //!< left 1-11, right 1-11
};

/*!
  \brief the shows of a game log, and how far each player has walked by
  each frame (rcssmonitor-web pose.ts gaitAt).
*/
/*!
  \brief where a figure is and how it moves at a time between frames: the
  idle, walk, run and sprint clips (in this order) mixed by the speed, each
  at the time of the distance walked over its own speed (rcssmonitor-web
  figures/gltf.ts).
*/
struct ProtoFigure {
    float x = 0.0f;
    float y = 0.0f;
    float body = 0.0f; //!< degree
    float times[4]; //!< seconds
    float weights[4];
};

class ProtoLog {
public:
    std::vector< ProtoFrame > frames_;
    //! meters walked, by frame * PROTO_PLAYERS + player; less when going
    //! backward, and a move over 2 m in a cycle is not counted.
    std::vector< float > distance_;

    bool load( const std::string & path );

    int size() const { return static_cast< int >( frames_.size() ); }

    //! the speed of a player (m/s)
    static float speed( const ProtoPlayer & p );

    /*!
      \brief a figure at a time.
      \param t frames from the first one (looped)
      \param i the player index
    */
    ProtoFigure figureAt( double t,
                          int i ) const;

    //! the ball at a time (frames from the first one, looped)
    void ballAt( double t,
                 float * x,
                 float * y ) const;

private:
    void computeGait();
};

#endif
