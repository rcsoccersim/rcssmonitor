// -*-c++-*-

/*!
  \file whistle.h
  \brief the referee's whistle: blown at the changes of the play mode of the
  frames shown. Header File.
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

#ifndef RCSSMONITOR_WHISTLE_H
#define RCSSMONITOR_WHISTLE_H

#include "disp_holder.h"
#include "whistle_rules.h"

#include <QIODevice>
#include <QMutex>
#include <QObject>
#include <QString>

#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

class QTimer;

class LogPlayer;

/*!
  \class WhistleMixer
  \brief the samples of the whistle's audio output (mono, 16 bits, RATE):
  silence, and a whistle when one is played, from its start. The output
  reads them as long as it is open (the pull mode of QAudioSink), so it
  never runs out of data, and is not opened or closed for a whistle.
*/
class WhistleMixer
    : public QIODevice {
public:
    //! the sample rate of the sounds (tools/make_whistle.py)
    static const int RATE = 44100;

private:
    mutable QMutex M_mutex;
    std::vector< qint16 > M_short;
    std::vector< qint16 > M_long;
    const std::vector< qint16 > * M_current; //!< the whistle playing, or null
    std::size_t M_position; //!< its next sample
    double M_gain; //!< of its samples, 0 to 1

public:
    WhistleMixer();

    /*!
      \brief the samples of a WAV file as tools/make_whistle.py makes it
      (PCM, mono, RATE, 16 bits)
      \return the samples, or none if it cannot be read or is not one
    */
    static std::vector< qint16 > readWav( const QString & path );

    //! the samples of the short and the long whistle
    void setWhistles( const std::vector< qint16 > & short_whistle,
                      const std::vector< qint16 > & long_whistle );

    //! play a whistle from its start, instead of the one playing; gain 0 to 1
    void play( const bool long_whistle,
               const double gain );
    //! silence from now on
    void stop();
    //! a whistle is playing
    bool isPlaying() const;

    bool isSequential() const override { return true; }
    qint64 bytesAvailable() const override;

protected:
    qint64 readData( char * data,
                     qint64 maxlen ) override;
    qint64 writeData( const char * data,
                      qint64 len ) override;
};

/*!
  \class WhistleSound
  \brief the output of the whistle's sounds: one audio output with a
  WhistleMixer (with Qt Multimedia), or a fake in the tests
*/
class WhistleSound {
public:
    virtual ~WhistleSound() = default;

    /*!
      \brief play a whistle once, stopping the one playing
      \param long_whistle the long whistle, or the short one
      \param volume 0 to 100, as the volume slider (perceived loudness)
    */
    virtual void play( const bool long_whistle,
                       const int volume ) = 0;

    //! stop the whistle playing
    virtual void stop() = 0;

    /*!
      \brief keep the audio output open, playing silence between the
      whistles, or close it. PipeWire suspends an output that has played
      nothing for 5 s, and an HDMI output (a display's speakers) that wakes
      up from it loses the start of the next sound (a second or more): a
      whistle after a quiet time was not heard. play() opens it too, if it
      is not held.
    */
    virtual void hold( const bool on ) = 0;
};

/*!
  \class Whistle
  \brief the referee's whistle (docs/plan/2026-11-22-00_v20-features.md §C):
  watches the frame shown (LogPlayer::indexUpdated()), and blows the
  whistle of whistleFor() when its play mode changes from the frame shown
  before. Only when the whistle is on in the options (Options::whistle()).

  - Log replay: only when the frame shown is the one after the frame shown
    before (playing, or a step forward). Not after a jump, a step back, or
    going to the first or the last frame. The play from the first frame
    blows the kick-off of the game if the log starts with one
    (whistleAtFirstFrame()): the logs have no frame before it.
  - Live and the delayed live view: an update may go on by more than one
    frame (MAX_LIVE_STEP at most). The changes in the frames passed blow
    the strongest whistle of them, once.

  The frame shown before is kept by its pointer, not by its index:
  DispHolder drops the oldest live frames (trimLiveFrames()), which moves
  the indexes, and a new log or a new connection makes new frames.

  The two and the three long whistles (the end of a half, of the game)
  play the long whistle again every LONG_REPEAT_INTERVAL. A whistle stops
  the one playing (playing a log fast).

  The sound output is made when it is first needed (the whistle is off by
  default): at a frame shown while the whistle is on, or at a whistle
  blown, so that the sounds are loaded before the first change. While the
  whistle is on, the output is kept open (WhistleSound::hold()).
*/
class Whistle
    : public QObject {

    Q_OBJECT

public:

    //! makes the sound output (null for none)
    typedef std::function< std::unique_ptr< WhistleSound >() > SoundFactory;

    //! the most frames that a live update goes on by to blow a whistle; more is a jump
    static const std::size_t MAX_LIVE_STEP;
    //! from the start of a long whistle to the next of the two or three (ms): its length and a pause
    static const int LONG_REPEAT_INTERVAL;

private:

    const DispHolder & M_disp_holder;
    const LogPlayer & M_log_player;

    SoundFactory M_sound_factory;
    std::unique_ptr< WhistleSound > M_sound;
    bool M_sound_made;

    //! the long whistles still to blow
    QTimer * M_repeat_timer;
    int M_repeats_left;

    //! the frame shown before (null if none)
    DispConstPtr M_last_frame;

    // not used
    Whistle() = delete;
    Whistle( const Whistle & ) = delete;
    Whistle & operator=( const Whistle & ) = delete;

public:

    /*!
      \param disp_holder the frames
      \param log_player whether live
      \param sound_factory makes the output of the sounds when it is first
      needed (createSound() in rcssmonitor); it may make none (a build
      without Qt Multimedia)
      \param parent the parent object
    */
    Whistle( const DispHolder & disp_holder,
             const LogPlayer & log_player,
             SoundFactory sound_factory,
             QObject * parent );
    ~Whistle() override;

    //! a build with Qt Multimedia: the whistle sounds
    static bool isAvailable();

    //! the sound of the whistle: one audio output with Qt Multimedia, or null
    static std::unique_ptr< WhistleSound > createSound();

    //! whether the sound output has been made
    bool hasSound() const { return static_cast< bool >( M_sound ); }

    /*!
      \brief the whistle when the frame shown changes to the one of an
      index from the frame shown before (the strongest one of the frames
      passed in live). None if the frames do not follow it.
    */
    WhistleKind whistleTo( const std::size_t index ) const;

public slots:

    //! forget the frame shown before (a new log or connection) and stop the whistle
    void clear();

    /*!
      \brief stop the whistle, and close and drop the sound output (made
      again when it is needed). Called before the main window is destroyed:
      closing an output may run the event loop for a while, which must not
      deliver events to the widgets being destroyed.
    */
    void releaseSound();

    /*!
      \brief keep the audio output open while the whistle is on in the
      options, or close it (WhistleSound::hold()). Called when the option
      changes; a frame shown calls it too.
    */
    void updateHold();

    /*!
      \brief the frame shown changed: blow the whistle if the play mode
      changed (LogPlayer::indexUpdated())
    */
    void handleIndexUpdated( size_t current,
                             size_t maximum );

    /*!
      \brief blow a whistle at the volume of the options, stopping the one
      playing (also to try the volume in the settings dialog)
    */
    void blow( const WhistleKind kind );

    //! stop the whistle playing, and the long ones still to blow
    void stop();

private:

    //! make the sound output if not yet
    void makeSound();

private slots:

    void repeatLong();
};

#endif
