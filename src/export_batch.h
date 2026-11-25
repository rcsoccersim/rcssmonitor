// -*-c++-*-

/*!
  \file export_batch.h
  \brief the export from the command line, without a window. Header File.
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

#ifndef RCSSMONITOR_EXPORT_BATCH_H
#define RCSSMONITOR_EXPORT_BATCH_H

#include "disp_holder.h"
#include "export_range.h"
#include "options.h"

#include <QString>
#include <QStringList>

#include <ostream>
#include <vector>

class QCommandLineParser;

/*!
  \class ExportBatch
  \brief the export from the command line (docs/plan/2026-11-22-03_frame-export.md
  4.7, 4.16): a game log read without a window, its ranges exported as
  images or videos as the export dialog does, the progress told on the
  standard error, and an exit code. Nothing is saved in the settings.

  The options (--export-video FILE or --export-images DIR, and the others
  of addOptions()) are read with the monitor's (Options::parseCmdLine()).
  A RANGE is START-END (cycles or stopped frames, 1234.3; either end may
  be empty), all, a part of the game (first-half, second-half, half-N,
  extra-N, penalties, game) or goals[:BEFORE:AFTER] (a range for each
  goal, in seconds).

  It runs on the platform offscreen (main() chooses it): the 2D view
  draws without a display, and the 3D view gets OpenGL from an X display
  when there is one (the desktop, or xvfb-run on a machine without one).
*/
class ExportBatch {
public:

    //! the size of the images by default
    static constexpr int DEFAULT_WIDTH = 1280;
    static constexpr int DEFAULT_HEIGHT = 720;
    //! the height of the view the images are laid out as by default (the scale: the height of the images / this)
    static constexpr double LAYOUT_HEIGHT = 720.0;

    //! a range of frames and its name (for the files of a range)
    struct NamedRange {
        ExportRange range;
        QString name;
    };

    //! add the options of the export to a parser
    static
    void addOptions( QCommandLineParser & parser );

    /*!
      \brief read the options of the export from a parser
      \param parser the parser that has processed the command line
      \param settings set to what was given (left as it is for the others)
      \param error set to the reason when an option is wrong
      \return false if an option is wrong
    */
    static
    bool readOptions( const QCommandLineParser & parser,
                      Options::BatchExportSettings * settings,
                      QString * error );

    /*!
      \brief the ranges of the texts in the frames of a game log
      \param texts the ranges given (none: all)
      \param holder the frames, the parameters and the goals
      \param ranges set to the ranges, in the order given (the goals in their order)
      \param error set to the reason when a text is wrong, or when there is no range
      \return false if a text is wrong or there is no range
    */
    static
    bool resolveRanges( const QStringList & texts,
                        const DispHolder & holder,
                        std::vector< NamedRange > * ranges,
                        QString * error );

    /*!
      \param settings the options given
      \param log where the progress and the result are told (the standard error)
    */
    ExportBatch( const Options::BatchExportSettings & settings,
                 std::ostream & log );

    /*!
      \brief read the game log and export it: the event loop runs until the end
      \param game_log the path of the game log (.rcg or .rcg.gz)
      \return the exit code: 0 when exported, 1 otherwise
    */
    int run( const QString & game_log );

    //! the frames read by run()
    const DispHolder & dispHolder() const { return M_disp_holder; }

private:

    Options::BatchExportSettings M_settings;
    std::ostream & M_log;
    DispHolder M_disp_holder;

    //! the files of the videos: one, or FILE-NAME.ext for each range
    QStringList videoFiles( const std::vector< NamedRange > & ranges ) const;

    //! tell an error, and the exit code of it
    int fail( const QString & message );
};

#endif
