// -*-c++-*-

/*!
  \file whistle.cpp
  \brief the referee's whistle: blown at the changes of the play mode of the
  frames shown. Source File.
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

#include "whistle.h"

#include "log_player.h"
#include "options.h"

#include <QFile>
#include <QMutexLocker>
#include <QTimer>
#include <QtEndian>

#ifdef HAVE_QT_MULTIMEDIA
#include <QAudio>
#include <QAudioFormat>
#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
#include <QAudioDevice>
#include <QAudioSink>
#include <QMediaDevices>
#else
#include <QAudioDeviceInfo>
#include <QAudioOutput>
#endif
#endif

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>

const std::size_t Whistle::MAX_LIVE_STEP = 20;
// the long whistle is 0.95 s (tools/make_whistle.py), then a pause of 0.35 s
const int Whistle::LONG_REPEAT_INTERVAL = 1300;

/*-------------------------------------------------------------------*/
WhistleMixer::WhistleMixer()
    : QIODevice(),
      M_current( nullptr ),
      M_position( 0 ),
      M_gain( 0.0 )
{
    // unbuffered: a buffer of QIODevice would read silence ahead, and delay
    // a whistle played by as much (16 KB, about 0.19 s)
    open( QIODevice::ReadOnly | QIODevice::Unbuffered );
}

/*-------------------------------------------------------------------*/
std::vector< qint16 >
WhistleMixer::readWav( const QString & path )
{
    QFile file( path );
    if ( ! file.open( QIODevice::ReadOnly ) )
    {
        return std::vector< qint16 >();
    }
    const QByteArray wav = file.readAll();
    if ( wav.size() < 12
         || wav.mid( 0, 4 ) != "RIFF"
         || wav.mid( 8, 4 ) != "WAVE" )
    {
        return std::vector< qint16 >();
    }

    // the chunks: "fmt " (PCM, mono, RATE, 16 bits), then "data"
    bool format = false;
    int pos = 12;
    while ( pos + 8 <= wav.size() )
    {
        const QByteArray id = wav.mid( pos, 4 );
        const quint32 size = qFromLittleEndian< quint32 >( wav.constData() + pos + 4 );
        pos += 8;
        if ( id == "fmt "
             && size >= 16
             && pos + 16 <= wav.size() )
        {
            const char * f = wav.constData() + pos;
            format = ( qFromLittleEndian< quint16 >( f ) == 1 // PCM
                       && qFromLittleEndian< quint16 >( f + 2 ) == 1 // mono
                       && qFromLittleEndian< quint32 >( f + 4 ) == quint32( RATE )
                       && qFromLittleEndian< quint16 >( f + 14 ) == 16 );
        }
        else if ( id == "data" )
        {
            if ( ! format )
            {
                break;
            }
            const qint64 bytes = std::min< qint64 >( size, wav.size() - pos );
            std::vector< qint16 > samples( static_cast< std::size_t >( bytes / 2 ) );
            for ( std::size_t i = 0; i < samples.size(); ++i )
            {
                samples[i] = qFromLittleEndian< qint16 >( wav.constData() + pos + 2 * i );
            }
            return samples;
        }
        pos += static_cast< int >( size + ( size & 1 ) );
    }
    return std::vector< qint16 >();
}

/*-------------------------------------------------------------------*/
void
WhistleMixer::setWhistles( const std::vector< qint16 > & short_whistle,
                           const std::vector< qint16 > & long_whistle )
{
    QMutexLocker lock( &M_mutex );
    M_short = short_whistle;
    M_long = long_whistle;
    M_current = nullptr;
}

/*-------------------------------------------------------------------*/
void
WhistleMixer::play( const bool long_whistle,
                    const double gain )
{
    QMutexLocker lock( &M_mutex );
    M_current = ( long_whistle ? &M_long : &M_short );
    M_position = 0;
    M_gain = std::min( 1.0, std::max( 0.0, gain ) );
}

/*-------------------------------------------------------------------*/
void
WhistleMixer::stop()
{
    QMutexLocker lock( &M_mutex );
    M_current = nullptr;
}

/*-------------------------------------------------------------------*/
bool
WhistleMixer::isPlaying() const
{
    QMutexLocker lock( &M_mutex );
    return M_current != nullptr;
}

/*-------------------------------------------------------------------*/
qint64
WhistleMixer::bytesAvailable() const
{
    // always: silence between the whistles
    return 1024 * 1024 + QIODevice::bytesAvailable();
}

/*-------------------------------------------------------------------*/
qint64
WhistleMixer::readData( char * data,
                        qint64 maxlen )
{
    QMutexLocker lock( &M_mutex );
    const qint64 count = maxlen / 2;
    for ( qint64 i = 0; i < count; ++i )
    {
        qint16 v = 0;
        if ( M_current )
        {
            if ( M_position < M_current->size() )
            {
                v = static_cast< qint16 >( std::lround( ( *M_current )[M_position] * M_gain ) );
                ++M_position;
            }
            if ( M_position >= M_current->size() )
            {
                M_current = nullptr;
            }
        }
        // the native byte order of the audio format
        std::memcpy( data + 2 * i, &v, 2 );
    }
    return count * 2;
}

/*-------------------------------------------------------------------*/
qint64
WhistleMixer::writeData( const char *,
                         qint64 )
{
    return -1;
}

#ifdef HAVE_QT_MULTIMEDIA

/*!
  resources/sounds/sounds.qrc: linked into the static library
  rcssmonitor_core, so registered explicitly (as the icons; outside a
  namespace: Q_INIT_RESOURCE declares the function of the resources)
*/
static
void
init_sound_resources()
{
    Q_INIT_RESOURCE( sounds );
}

namespace {

#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
typedef QAudioSink AudioOutput;
#else
typedef QAudioOutput AudioOutput;
#endif

/*!
  \class StreamWhistle
  \brief the whistle's sounds on one audio output (QAudioSink, or
  QAudioOutput of Qt5), kept open while held, reading the samples from a
  WhistleMixer: silence between the whistles.

  QSoundEffect (Qt 6.4, on PipeWire) opened an output of its own for every
  whistle, and closed it at the end: an output starts only after about
  0.125 s is buffered, and the end of a sound was dropped; each output
  changed the quantum of the HDMI output (2048 and 1024 frames), and on a
  real HDMI output whistles were cut or not heard, more after the three
  long whistles of the end of the game. With one output, nothing is opened
  or closed for a whistle.

  Opening and closing an output may run the event loop for a while (the
  destructor of QAudioSink of Qt 6.4 and of QAudioOutput of Qt5 on
  PulseAudio do): a call from there (a frame shown, a whistle) does not
  open or close it again while it is being opened or closed.
*/
class StreamWhistle
    : public WhistleSound {
private:
    WhistleMixer M_mixer;
    QAudioFormat M_format;
    std::unique_ptr< AudioOutput > M_output;
    bool M_warned; //!< that there is no output device
    bool M_busy; //!< the output being opened or closed
    bool M_close_later; //!< closed after it is opened (close() while opening)

public:
    StreamWhistle()
        : M_warned( false ),
          M_busy( false ),
          M_close_later( false )
      {
          init_sound_resources();
          M_mixer.setWhistles( WhistleMixer::readWav( QStringLiteral( ":/sounds/whistle_short.wav" ) ),
                               WhistleMixer::readWav( QStringLiteral( ":/sounds/whistle_long.wav" ) ) );
          M_format.setSampleRate( WhistleMixer::RATE );
          M_format.setChannelCount( 1 );
#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
          M_format.setSampleFormat( QAudioFormat::Int16 );
#else
          M_format.setSampleSize( 16 );
          M_format.setCodec( QStringLiteral( "audio/pcm" ) );
          M_format.setByteOrder( QSysInfo::ByteOrder == QSysInfo::LittleEndian
                                 ? QAudioFormat::LittleEndian
                                 : QAudioFormat::BigEndian );
          M_format.setSampleType( QAudioFormat::SignedInt );
#endif
      }

    ~StreamWhistle() override
      {
          close();
      }

    void play( const bool long_whistle,
               const int volume ) override
      {
          open(); // if not held
          // the slider's volume is the loudness heard: to the linear scale
          // of the samples, as Qt recommends for a volume slider
          const double v = std::min( 100, std::max( 0, volume ) ) / 100.0;
          M_mixer.play( long_whistle,
                        QAudio::convertVolume( v,
                                               QAudio::LogarithmicVolumeScale,
                                               QAudio::LinearVolumeScale ) );
      }

    void stop() override
      {
          M_mixer.stop();
      }

    void hold( const bool on ) override
      {
          if ( on )
          {
              open();
          }
          else
          {
              close();
          }
      }

private:

    //! open the output on the default device, if it is not open (or it stopped by an error)
    void open()
      {
          if ( M_busy
               || ( M_output
                    && M_output->state() != QAudio::StoppedState ) )
          {
              return;
          }
          M_busy = true;
          M_close_later = false;
          M_output.reset(); // stopped by an error
#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
          const QAudioDevice device = QMediaDevices::defaultAudioOutput();
#else
          // Qt5 finds the devices by the audio plugins of Qt Multimedia
          // (libqt5multimedia5-plugins on Debian and Ubuntu)
          const QAudioDeviceInfo device = QAudioDeviceInfo::defaultOutputDevice();
#endif
          if ( device.isNull() )
          {
              if ( ! M_warned )
              {
                  std::cerr << "rcssmonitor: no audio output for the referee's whistle"
#if QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
                            << " (Qt5 needs the audio plugins of Qt Multimedia, e.g. libqt5multimedia5-plugins)"
#endif
                            << std::endl;
                  M_warned = true;
              }
              M_busy = false;
              return;
          }
          std::unique_ptr< AudioOutput > output( new AudioOutput( device, M_format ) );
          output->start( &M_mixer );
          M_output = std::move( output );
          M_busy = false;
          if ( M_close_later )
          {
              close();
          }
      }

    void close()
      {
          M_mixer.stop();
          if ( M_busy )
          {
              M_close_later = true;
              return;
          }
          M_close_later = false;
          // taken out first: a call while it is closed sees none
          std::unique_ptr< AudioOutput > output = std::move( M_output );
          if ( output )
          {
              M_busy = true;
              output->stop();
              output.reset();
              M_busy = false;
          }
      }
};

}

#endif

/*-------------------------------------------------------------------*/
Whistle::Whistle( const DispHolder & disp_holder,
                  const LogPlayer & log_player,
                  SoundFactory sound_factory,
                  QObject * parent )
    : QObject( parent ),
      M_disp_holder( disp_holder ),
      M_log_player( log_player ),
      M_sound_factory( std::move( sound_factory ) ),
      M_sound(),
      M_sound_made( false ),
      M_repeat_timer( new QTimer( this ) ),
      M_repeats_left( 0 )
{
    M_repeat_timer->setInterval( LONG_REPEAT_INTERVAL );
    connect( M_repeat_timer, &QTimer::timeout,
             this, &Whistle::repeatLong );
}

/*-------------------------------------------------------------------*/
Whistle::~Whistle()
{
    stop();
}

/*-------------------------------------------------------------------*/
bool
Whistle::isAvailable()
{
#ifdef HAVE_QT_MULTIMEDIA
    return true;
#else
    return false;
#endif
}

/*-------------------------------------------------------------------*/
std::unique_ptr< WhistleSound >
Whistle::createSound()
{
#ifdef HAVE_QT_MULTIMEDIA
    return std::unique_ptr< WhistleSound >( new StreamWhistle() );
#else
    return std::unique_ptr< WhistleSound >();
#endif
}

/*-------------------------------------------------------------------*/
void
Whistle::makeSound()
{
    if ( M_sound_made )
    {
        return;
    }
    M_sound_made = true;
    if ( M_sound_factory )
    {
        M_sound = M_sound_factory();
    }
}

/*-------------------------------------------------------------------*/
WhistleKind
Whistle::whistleTo( const std::size_t index ) const
{
    const DispCont & frames = M_disp_holder.dispCont();
    if ( ! M_last_frame
         || index >= frames.size() )
    {
        return WhistleKind::None;
    }

    // the frame shown before: just before it in log replay, and within
    // MAX_LIVE_STEP frames in live
    const bool live = M_log_player.isLiveMode();
    const std::size_t max_step = ( live ? MAX_LIVE_STEP : 1 );
    std::size_t from = index;
    for ( std::size_t step = 1; step <= max_step && step <= index; ++step )
    {
        if ( frames[index - step] == M_last_frame )
        {
            from = index - step;
            break;
        }
    }

    WhistleKind kind = WhistleKind::None;
    for ( std::size_t i = from + 1; i <= index && from < index; ++i )
    {
        kind = strongerWhistle( kind,
                                whistleFor( frames[i - 1]->pmode_,
                                            frames[i]->pmode_,
                                            static_cast< int >( frames[i]->show_.time_ ) ) );
    }

    // the play from the first frame of a log: the kick-off of the game,
    // which has no frame before it
    if ( ! live
         && from == 0
         && from < index )
    {
        kind = strongerWhistle( kind, whistleAtFirstFrame( frames[0]->pmode_ ) );
    }
    return kind;
}

/*-------------------------------------------------------------------*/
void
Whistle::clear()
{
    stop();
    M_last_frame.reset();
}

/*-------------------------------------------------------------------*/
void
Whistle::releaseSound()
{
    stop();
    M_sound.reset();
    M_sound_made = false;
}

/*-------------------------------------------------------------------*/
void
Whistle::updateHold()
{
    if ( Options::instance().whistle() )
    {
        makeSound();
        if ( M_sound )
        {
            M_sound->hold( true );
        }
    }
    else if ( M_sound )
    {
        M_sound->hold( false );
    }
}

/*-------------------------------------------------------------------*/
void
Whistle::handleIndexUpdated( size_t current,
                             size_t /* maximum */ )
{
    const DispCont & frames = M_disp_holder.dispCont();
    if ( current >= frames.size() )
    {
        M_last_frame.reset();
        return;
    }

    // the frame is kept also while the whistle is off, so that it blows
    // at the next change after it is turned on
    WhistleKind kind = WhistleKind::None;
    if ( Options::instance().whistle() )
    {
        updateHold(); // loaded and kept open before the first change
        kind = whistleTo( current );
    }
    M_last_frame = frames[current];

    if ( kind != WhistleKind::None )
    {
        blow( kind );
    }
}

/*-------------------------------------------------------------------*/
void
Whistle::blow( const WhistleKind kind )
{
    stop();

    if ( kind == WhistleKind::None )
    {
        return;
    }

    makeSound();
    if ( ! M_sound )
    {
        return;
    }

    M_repeats_left = ( kind == WhistleKind::LongTwice ? 1
                       : kind == WhistleKind::LongThrice ? 2
                       : 0 );
    M_sound->play( kind != WhistleKind::Short, Options::instance().whistleVolume() );
    if ( M_repeats_left > 0 )
    {
        M_repeat_timer->start();
    }
}

/*-------------------------------------------------------------------*/
void
Whistle::stop()
{
    M_repeat_timer->stop();
    M_repeats_left = 0;
    if ( M_sound )
    {
        M_sound->stop();
    }
}

/*-------------------------------------------------------------------*/
void
Whistle::repeatLong()
{
    if ( M_repeats_left <= 0
         || ! M_sound )
    {
        M_repeat_timer->stop();
        return;
    }

    --M_repeats_left;
    M_sound->play( true, Options::instance().whistleVolume() );
    if ( M_repeats_left <= 0 )
    {
        M_repeat_timer->stop();
    }
}
