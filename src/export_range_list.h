// -*-c++-*-

/*!
  \file export_range_list.h
  \brief the ranges to export, as the user gives them. Header File.
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

#ifndef RCSSMONITOR_EXPORT_RANGE_LIST_H
#define RCSSMONITOR_EXPORT_RANGE_LIST_H

#include "export_range.h"

#include <QObject>
#include <QString>
#include <QVector>

/*!
  \struct ExportRangeSpec
  \brief a range as the user gives it: the positions of the start and the
  end (ExportRange::parsePosition(); empty for the first and the last
  frames), and its name (for the directories of its images)
*/
struct ExportRangeSpec {
    QString start;
    QString end;
    QString name;

    bool operator==( const ExportRangeSpec & other ) const
      {
          return start == other.start && end == other.end && name == other.name;
      }
};

/*!
  \class ExportRangeList
  \brief the ranges to export (docs/plan/2026-11-22-03_frame-export.md 4.4):
  one or more, one of them current. The main window marks the start and the
  end of the current one, the export dialog edits them, and the slider of
  the playback shows them. Kept only for the game log open (not in the
  settings): cleared when another log is opened or a server is connected.
*/
class ExportRangeList
    : public QObject {

    Q_OBJECT

private:

    QVector< ExportRangeSpec > M_ranges; //!< one at least
    int M_current;

public:

    explicit
    ExportRangeList( QObject * parent = nullptr );

    const QVector< ExportRangeSpec > & ranges() const { return M_ranges; }
    int size() const { return M_ranges.size(); }
    int current() const { return M_current; }
    const ExportRangeSpec & currentRange() const { return M_ranges[M_current]; }

    void setCurrent( const int index );

    //! the start, the end or the name of the current range
    void setStart( const QString & text );
    void setEnd( const QString & text );
    void setName( const QString & name );
    void setCurrentRange( const ExportRangeSpec & spec );

    //! a range at the end, made current
    void add( const ExportRangeSpec & spec );

    //! remove a range: the last one left is made the whole log
    void remove( const int index );

    //! move a range to another place, keeping it current if it was
    void move( const int from,
               const int to );

    //! the ranges given (none: one of the whole log), the first current
    void setRanges( const QVector< ExportRangeSpec > & ranges );

    //! one range of the whole log
    void clear();

    //! whether it is one range of the whole log
    bool isCleared() const;

    //! a name not used yet: base1, base2, ...
    QString newName( const QString & base = QStringLiteral( "range" ) ) const;

    /*!
      \brief a name as a part of a file name: the letters, the digits and
      ".-_" kept, the others as "_"
    */
    static
    QString safeName( const QString & name );

    /*!
      \brief the frames of a range given
      \param error set to the reason when it is not valid (may be null)
      \param start_snapped, end_snapped set when an end was moved to a frame (may be null)
      \return the range, invalid with the reason
    */
    static
    ExportRange resolve( const ExportRangeSpec & spec,
                         const DispCont & frames,
                         QString * error,
                         bool * start_snapped = nullptr,
                         bool * end_snapped = nullptr );

signals:

    //! a range, the ranges, or the current one changed
    void changed();
};

#endif
