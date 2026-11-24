// -*-c++-*-

/*!
  \file export_dialog.h
  \brief the dialog of File > Export Images / Video. Header File.
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

#ifndef RCSSMONITOR_EXPORT_DIALOG_H
#define RCSSMONITOR_EXPORT_DIALOG_H

#include "export_range.h"

#include <QDialog>
#include <QPointer>
#include <QSize>
#include <QStringList>

#include <cstddef>
#include <vector>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QProgressDialog;
class QPushButton;
class QSpinBox;

class DispHolder;
class ExportJob;
class FieldCanvas;

/*!
  \class ExportDialog
  \brief File > Export Images / Video (docs/plan/2026-11-22-03_frame-export.md
  4.4, 4.6, S8, D6): the frames of a range of the game log as images.
  Modeless: the range can be taken from the position shown while it is
  open. The start and the end are positions (ExportRange: 1234 or 1234.3;
  empty for the first and the last frames of the log). The images are
  drawn by the 2D view (FieldCanvas::renderFrame()), in a size and with
  the texts scaled or not, at a rate (fps and speed: ExportTiming), and
  named by their numbers or by the cycles. Before the export, the files
  that are there are told once, and the disk space is estimated.
  While exporting, a progress dialog (application modal) can stop it.
*/
class ExportDialog
    : public QDialog {

    Q_OBJECT

private:

    DispHolder & M_disp_holder;
    FieldCanvas & M_canvas;

    QLineEdit * M_start_edit;
    QLineEdit * M_end_edit;
    QLabel * M_start_info;
    QLabel * M_end_info;

    QComboBox * M_size_combo;
    QSpinBox * M_width_spin;
    QSpinBox * M_height_spin;
    QCheckBox * M_scale_texts_check;

    QDoubleSpinBox * M_fps_spin;
    QDoubleSpinBox * M_speed_spin;

    QLineEdit * M_dir_edit;
    QLineEdit * M_prefix_edit;
    QComboBox * M_format_combo;
    QCheckBox * M_name_by_cycle_check;

    QLabel * M_summary_label;
    QLabel * M_message_label;
    QPushButton * M_export_button;

    bool M_allowed; //!< false while connected to a server (S1)
    QPointer< ExportJob > M_job;
    QPointer< QProgressDialog > M_progress;

public:

    ExportDialog( DispHolder & disp_holder,
                  FieldCanvas & canvas,
                  QWidget * parent = nullptr );
    ~ExportDialog() override;

    //! the range of the fields, or an invalid one with the reason
    ExportRange range( QString * error ) const;

    //! the frames a second of the simulation step: framesPerImage() of the fields
    double framesPerImage() const;

    //! the frame of each image of the range
    std::vector< std::size_t > imageFrames( const ExportRange & range ) const;

    //! the size of the images (pixels)
    QSize imageSize() const;

    //! the device pixel ratio of the images: the texts as large as in the view, or 1
    qreal imageRatio() const;

    //! the file names of the images of the frames (numbers, or cycles)
    QStringList imageNames( const std::vector< std::size_t > & frames ) const;

    //! naming by the cycles can be chosen: one frame an image
    bool canNameByCycle() const;

    QString directory() const;

    QString summaryText() const;
    QString messageText() const;
    bool isExporting() const;

public slots:

    void setStart( const QString & text );
    void setEnd( const QString & text );
    void setStartToCurrent();
    void setEndToCurrent();
    //! the whole log: both fields empty
    void setAll();

    void setDirectory( const QString & dir );

    //! false while connected to a server: the export is not started (S1)
    void setExportAllowed( const bool allowed );

    //! the summary of the range and the images, and whether it can be exported
    void updateSummary();

    /*!
      \brief export with the fields
      \param ask ask before overwriting files and when the disk may not have
      enough space (false: no question, overwrite; for the tests)
      \return true if started
    */
    bool startExport( const bool ask = true );

    void abortExport();

signals:

    //! the playback should stop: the frame shown stays while exporting
    void stopRequested();

    //! the end of an export: the result
    void exportFinished( bool ok,
                         const QString & message );

private:

    void createWidgets();
    void readSettings();
    void writeSettings() const;

    void chooseDirectory();
    void showMessage( const QString & text,
                      const bool error );

    //! the start or the end of a field: the index, or INVALID_INDEX with the reason
    std::size_t positionIndex( const QLineEdit * edit,
                               const bool start,
                               bool * snapped,
                               QString * error ) const;

    void handleFinished( bool ok,
                         const QString & message );
};

#endif
