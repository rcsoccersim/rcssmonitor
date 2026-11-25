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
#include "ffmpeg_sink.h"

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
class QListWidget;
class QMenu;
class QProgressDialog;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QToolButton;

class DispHolder;
class ExportJob;
struct ExportPart;
class ExportRangeList;
class FieldCanvas;
class FieldView3D;
class OffscreenView3D;
struct ExportRangeSpec;

/*!
  \class ExportDialog
  \brief File > Export Images / Video (docs/plan/2026-11-22-03_frame-export.md
  4.4, 4.6, S8, D6 to D9): the frames of ranges of the game log as images.
  Modeless: the ranges can be taken from the position shown while it is
  open.

  The ranges are those of an ExportRangeList, shared with the marks of the
  main window and the bands of the slider: one or more, one current. The
  start and the end are positions (ExportRange: 1234 or 1234.3; empty for
  the first and the last frames). The presets give the halves, the goal
  scenes (one or all) and the frames around the position shown. Several
  ranges are written each in its own directory, or in one sequence.

  The images are drawn by the 2D view (FieldCanvas::renderFrame()) or by
  the 3D view drawn offscreen (OffscreenView3D: between the frames, the
  camera starting as the view's), the view shown when the dialog is opened
  (D17), in a size and with the texts scaled or not, at a rate (fps and
  speed: ExportTiming; an fps for each view). They are written as image files, named by their numbers
  or by the cycles, or as a video by an external ffmpeg (FfmpegSink: H.264
  at a quality and a speed, D14, D15; a video for each range, or one for
  all). Before the export, the files that are there are told once, and the
  disk space of the images is estimated. While exporting, a progress
  dialog (application modal) can stop it.
*/
class ExportDialog
    : public QDialog {

    Q_OBJECT

private:

    DispHolder & M_disp_holder;
    FieldCanvas & M_canvas;
    ExportRangeList & M_ranges;

    QListWidget * M_range_list;
    QPushButton * M_add_button;
    QPushButton * M_remove_button;
    QPushButton * M_up_button;
    QPushButton * M_down_button;
    QLineEdit * M_name_edit;
    QToolButton * M_presets_button;
    QMenu * M_presets_menu;

    QLineEdit * M_start_edit;
    QLineEdit * M_end_edit;
    QLabel * M_start_info;
    QLabel * M_end_info;

    QDoubleSpinBox * M_goal_before_spin;
    QDoubleSpinBox * M_goal_after_spin;
    QDoubleSpinBox * M_around_before_spin;
    QDoubleSpinBox * M_around_after_spin;

    QRadioButton * M_view_2d_radio;
    QRadioButton * M_view_3d_radio;
    const FieldView3D * M_view_3d; //!< the 3D view of the window: its size and its camera (null: none yet)
    OffscreenView3D * M_offscreen_3d; //!< draws the 3D images (null: not built with the 3D view)
    QString M_view_3d_reason; //!< why the 3D view cannot be drawn (empty: it can)
    double M_other_fps; //!< the fps of the view not chosen (one for each view)

    QComboBox * M_size_combo;
    QSpinBox * M_width_spin;
    QSpinBox * M_height_spin;
    QCheckBox * M_scale_texts_check;

    QDoubleSpinBox * M_fps_spin;
    QDoubleSpinBox * M_speed_spin;
    QPushButton * M_playback_speed_button;

    QRadioButton * M_images_radio;
    QRadioButton * M_video_radio;
    QWidget * M_images_widget;
    QWidget * M_video_widget;
    QLineEdit * M_video_edit;
    QSpinBox * M_crf_spin;
    QComboBox * M_preset_combo;
    QLineEdit * M_ffmpeg_edit;
    QLabel * M_ffmpeg_label;
    FfmpegSink::Info M_ffmpeg_info;
    bool M_ffmpeg_probed;

    QLineEdit * M_dir_edit;
    QLineEdit * M_prefix_edit;
    QComboBox * M_format_combo;
    QCheckBox * M_name_by_cycle_check;
    QRadioButton * M_separate_radio;
    QRadioButton * M_one_sequence_radio;

    QLabel * M_summary_label;
    QLabel * M_message_label;
    QPushButton * M_export_button;

    bool M_allowed; //!< false while connected to a server (S1)
    QPointer< ExportJob > M_job;
    bool M_job_video; //!< the export is of videos
    QPointer< QProgressDialog > M_progress;

public:

    ExportDialog( DispHolder & disp_holder,
                  FieldCanvas & canvas,
                  ExportRangeList & ranges,
                  QWidget * parent = nullptr );
    ~ExportDialog() override;

    //! the current range, or an invalid one with the reason
    ExportRange range( QString * error ) const;

    //! all the ranges, or none with the reason of the first wrong one
    std::vector< ExportRange > ranges( QString * error ) const;

    //! the frames one image advances: framesPerImage() of the fields
    double framesPerImage() const;

    //! the frame of each image of a range
    std::vector< std::size_t > imageFrames( const ExportRange & range ) const;

    //! 3D: the position of each image of a range, between the frames
    std::vector< double > imagePositions( const ExportRange & range ) const;

    //! the 3D view draws the images (else the 2D view)
    bool isView3D() const;

    /*!
      \brief what the 3D images need: the window's 3D view (its size and its
      camera; null before it is made) and what draws them offscreen (null:
      none). The 3D view can be chosen when the images can be drawn here.
    */
    void setView3DSources( const FieldView3D * view,
                           OffscreenView3D * offscreen );

    //! the size of the images (pixels)
    QSize imageSize() const;

    //! the device pixel ratio of the images: the view enlarged (texts, logos, lines), or 1
    qreal imageRatio() const;

    //! the file names of the images of the frames (numbers, or cycles)
    QStringList imageNames( const std::vector< std::size_t > & frames ) const;

    /*!
      \brief the file names of the images of all the ranges: each range in
      its own directory, or all in one sequence
      \param error set to the reason if the names repeat (may be null)
      \return the names, or none
    */
    QStringList allImageNames( const std::vector< std::vector< std::size_t > > & frames,
                               QString * error ) const;

    //! naming by the cycles can be chosen: one frame an image
    bool canNameByCycle() const;

    //! the parts of the game and the goals in the log (the presets)
    std::vector< ExportPeriod > periods() const;
    std::vector< ExportGoal > goals() const;

    QString directory() const;

    //! a video (by ffmpeg), or images
    bool isVideo() const;

    //! the video file: with .mp4 when it has no extension
    QString videoFile() const;

    /*!
      \brief the video files of the ranges: one for all, or one for each
      range (FILE-NAME.ext by the names of the ranges)
    */
    QStringList videoFiles( const int ranges ) const;

    //! what the ffmpeg of the field can do (probeFfmpeg())
    const FfmpegSink::Info & ffmpegInfo() const { return M_ffmpeg_info; }

    QString summaryText() const;
    QString messageText() const;
    bool isExporting() const;

public slots:

    //! the view that draws the images: the 3D view (when it can be drawn), or the 2D view
    void chooseView3D( const bool on );

    void setStart( const QString & text );
    void setEnd( const QString & text );
    void setStartToCurrent();
    void setEndToCurrent();
    //! the current range over the whole log: both ends empty (the button Clear)
    void clearRange();

    //! the presets: the current range a part of the game, a goal scene, around the position shown
    void choosePeriod( const int index );
    void chooseGoal( const int index );
    void chooseAround();
    //! a range for each goal (in place of the whole log, or after the ranges)
    void chooseAllGoals();

    //! a range at the end (the whole log), made current
    void addRange();
    void removeRange();
    void moveRangeUp();
    void moveRangeDown();

    void setDirectory( const QString & dir );

    //! a video or images
    void setVideo( const bool video );
    void setVideoFile( const QString & file );
    //! the path of ffmpeg (empty: in the PATH), probed again
    void setFfmpeg( const QString & path );
    //! run the ffmpeg of the field: its version and its H.264 encoder
    void probeFfmpeg();

    //! the speed of the export as the monitor plays the log: the step / the interval of the playback
    void setPlaybackSpeed();

    //! false while connected to a server: the export is not started (S1)
    void setExportAllowed( const bool allowed );

    //! the ranges, the summary, and whether it can be exported
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
    QWidget * createRangeBox();
    void readSettings();
    void writeSettings() const;

    void buildPresetsMenu();

    //! the fields and the list from the ranges
    void showRanges();

    void chooseDirectory();
    void chooseVideoFile();
    void chooseFfmpeg();
    //! the fields of the images or of the video
    void showOutput();

    //! the view chosen: its fps in the field
    void showView();

    //! the size of the view shown (pixels) and its height (logical pixels): the 2D view or the 3D view
    QSize viewPixels() const;
    int viewHeight() const;
    void showMessage( const QString & text,
                      const bool error );

    //! why the video cannot be made (ffmpeg, the extension), or an empty text
    QString videoError() const;

    /*!
      \brief the images to write: their names, the directory (the files
      there asked about), and the bytes they may take (the disk space asked about)
      \return false if not exported (told in the message)
    */
    bool prepareImages( const std::vector< std::size_t > & frames,
                        const std::vector< std::vector< std::size_t > > & by_range,
                        const bool ask,
                        QStringList * names,
                        QString * dir_out,
                        qint64 * bytes );

    /*!
      \brief the videos to write: ffmpeg, the files (those there asked about),
      and a part for each video
      \return false if not exported (told in the message)
    */
    bool prepareVideos( const std::vector< std::vector< std::size_t > > & by_range,
                        const std::vector< std::size_t > & frames,
                        const bool ask,
                        std::vector< ExportPart > * parts );

    //! the names of the first ranges in the names of files: safe, and made unique (name, name-2, ...)
    QStringList rangeFileNames( const int count ) const;

    //! the text of a range in the list
    QString rangeText( const ExportRangeSpec & spec ) const;

    //! the positions of a range of frames, as the fields have them
    ExportRangeSpec specOf( const ExportRange & range,
                            const QString & name ) const;

    void handleFinished( bool ok,
                         const QString & message );
};

#endif
