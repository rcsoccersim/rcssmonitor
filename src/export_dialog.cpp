// -*-c++-*-

/*!
  \file export_dialog.cpp
  \brief the dialog of File > Export Images / Video. Source File.
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

#include "export_dialog.h"

#include "disp_holder.h"
#include "export_job.h"
#include "export_range_list.h"
#include "export_timing.h"
#include "field_canvas.h"
#include "image_sequence_sink.h"
#include "options.h"
#include "ui_palette.h"

#ifdef HAVE_3D_VIEW
#include "cameras_3d.h"
#include "field_view_3d.h"
#include "offscreen_view_3d.h"
#endif

#include <QBuffer>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontMetrics>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QImageWriter>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QRadioButton>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStorageInfo>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <memory>
#include <set>

namespace {

//! the sizes offered: the key kept in the settings, and the size (empty: as the view, or custom)
struct SizeChoice {
    const char * key;
    QSize size;
};

const SizeChoice SIZES[] = {
    { "view", QSize() },
    { "1280x720", QSize( 1280, 720 ) },
    { "1920x1080", QSize( 1920, 1080 ) },
    { "3840x2160", QSize( 3840, 2160 ) },
    { "custom", QSize() },
};

//! the digits of a number
int
digitsOf( const unsigned long long value )
{
    return static_cast< int >( QString::number( value ).size() );
}

//! a size of bytes to read: KB, MB or GB
QString
bytesText( const qint64 bytes )
{
    if ( bytes < 1024 * 1024 ) return QObject::tr( "%1 KB" ).arg( std::max< qint64 >( 1, bytes / 1024 ) );
    if ( bytes < 1024LL * 1024 * 1024 ) return QObject::tr( "%1 MB" ).arg( bytes / ( 1024.0 * 1024.0 ), 0, 'f', 1 );
    return QObject::tr( "%1 GB" ).arg( bytes / ( 1024.0 * 1024.0 * 1024.0 ), 0, 'f', 2 );
}

//! the time left to read: about 40 s, about 3 min
QString
leftText( const qint64 msecs )
{
    const qint64 secs = std::max< qint64 >( 1, ( msecs + 500 ) / 1000 );
    if ( secs < 60 ) return QObject::tr( "about %1 s left" ).arg( secs );
    return QObject::tr( "about %1 min left" ).arg( ( secs + 30 ) / 60 );
}

//! the extensions of the videos: their formats (D14)
const char * const VIDEO_EXTENSIONS[] = { "mp4", "mov", "mkv" };

//! the nearest directory that is there: the free space of where the images go
QString
existingDirectory( const QString & dir )
{
    QFileInfo info( dir );
    while ( ! info.exists() && ! info.absolutePath().isEmpty() && info.absoluteFilePath() != info.absolutePath() )
    {
        info = QFileInfo( info.absolutePath() );
    }
    return info.absoluteFilePath();
}

}

/*-------------------------------------------------------------------*/
ExportDialog::ExportDialog( DispHolder & disp_holder,
                            FieldCanvas & canvas,
                            ExportRangeList & ranges,
                            QWidget * parent )
    : QDialog( parent ),
      M_disp_holder( disp_holder ),
      M_canvas( canvas ),
      M_ranges( ranges ),
      M_view_3d( nullptr ),
      M_offscreen_3d( nullptr ),
      M_other_fps( ExportTiming::DEFAULT_FPS_3D ),
      M_ffmpeg_probed( false ),
      M_allowed( true ),
      M_job_video( false )
{
    this->setWindowTitle( tr( "Export Images / Video" ) );
    createWidgets();
    readSettings();
    // the 3D view, once its sources are given (setView3DSources())
    setView3DSources( nullptr, nullptr );
    connect( &M_ranges, &ExportRangeList::changed,
             this, &ExportDialog::showRanges );
    showRanges();
    this->resize( std::max( 680, this->sizeHint().width() ), this->sizeHint().height() );
}

/*-------------------------------------------------------------------*/
ExportDialog::~ExportDialog()
{
    if ( M_job )
    {
        M_job->abort();
    }
}

/*-------------------------------------------------------------------*/
QWidget *
ExportDialog::createRangeBox()
{
    QGroupBox * box = new QGroupBox( tr( "Ranges" ) );
    QVBoxLayout * top = new QVBoxLayout( box );

    // the list of the ranges, with the buttons to edit it
    {
        M_range_list = new QListWidget();
        M_range_list->setObjectName( "ranges" );
        M_range_list->setMaximumHeight( M_range_list->fontMetrics().height() * 5 + 8 );
        connect( M_range_list, &QListWidget::currentRowChanged,
                 &M_ranges, &ExportRangeList::setCurrent );

        const auto button = [this]( const QString & text, const char * name, const QString & tip, void ( ExportDialog::*slot )() ) {
            QPushButton * b = new QPushButton( text );
            b->setObjectName( name );
            b->setToolTip( tip );
            b->setAutoDefault( false );
            connect( b, &QPushButton::clicked, this, slot );
            return b;
        };
        M_add_button = button( tr( "Add" ), "add_range", tr( "A range of the whole log at the end" ), &ExportDialog::addRange );
        M_remove_button = button( tr( "Remove" ), "remove_range", tr( "Remove the range chosen" ), &ExportDialog::removeRange );
        M_up_button = button( tr( "Up" ), "range_up", tr( "Move the range chosen up" ), &ExportDialog::moveRangeUp );
        M_down_button = button( tr( "Down" ), "range_down", tr( "Move the range chosen down" ), &ExportDialog::moveRangeDown );

        QVBoxLayout * buttons = new QVBoxLayout();
        buttons->addWidget( M_add_button );
        buttons->addWidget( M_remove_button );
        buttons->addWidget( M_up_button );
        buttons->addWidget( M_down_button );
        buttons->addStretch( 1 );
        QHBoxLayout * row = new QHBoxLayout();
        row->addWidget( M_range_list, 1 );
        row->addLayout( buttons );
        top->addLayout( row );
    }

    QFormLayout * form = new QFormLayout();
    top->addLayout( form );

    // the name, and the presets
    {
        M_name_edit = new QLineEdit();
        M_name_edit->setObjectName( "range_name" );
        M_name_edit->setToolTip( tr( "The name of the range: the directory of its images, when the ranges are each in their own" ) );
        M_name_edit->setMaximumWidth( 200 );
        connect( M_name_edit, &QLineEdit::textEdited,
                 &M_ranges, &ExportRangeList::setName );

        M_presets_menu = new QMenu( this );
        M_presets_menu->setObjectName( "presets_menu" );
        connect( M_presets_menu, &QMenu::aboutToShow,
                 this, &ExportDialog::buildPresetsMenu );
        M_presets_button = new QToolButton();
        M_presets_button->setObjectName( "presets" );
        M_presets_button->setText( tr( "Presets" ) );
        M_presets_button->setToolTip( tr( "The whole log, a half, a goal scene, all the goals, or around the position shown" ) );
        M_presets_button->setPopupMode( QToolButton::InstantPopup );
        M_presets_button->setMenu( M_presets_menu );

        // as Clear the Range of the main window (D12)
        QPushButton * clear = new QPushButton( tr( "Clear" ) );
        clear->setObjectName( "clear_range" );
        clear->setToolTip( tr( "Clear the start and the end: the whole log.\n"
                               "In the main window: File > Export Images / Video > Clear the Range." ) );
        clear->setAutoDefault( false );
        connect( clear, &QPushButton::clicked, this, &ExportDialog::clearRange );

        QHBoxLayout * row = new QHBoxLayout();
        row->addWidget( M_name_edit );
        row->addWidget( clear );
        row->addWidget( M_presets_button );
        row->addStretch( 1 );
        form->addRow( tr( "Name:" ), row );
    }

    // the start and the end of the current range
    const auto position = [this, form]( const QString & label,
                                        const char * name,
                                        const QString & placeholder,
                                        QLineEdit ** edit,
                                        QLabel ** info,
                                        void ( ExportRangeList::*set )( const QString & ),
                                        void ( ExportDialog::*now )(),
                                        const QString & now_tip ) {
        *edit = new QLineEdit();
        ( *edit )->setObjectName( name );
        ( *edit )->setPlaceholderText( placeholder );
        ( *edit )->setToolTip( tr( "A cycle (e.g. 1234), or a cycle and a stopped frame (e.g. 1234.3, as the status bar shows it)." ) );
        // as wide for the start and the end, however long the texts beside them
        ( *edit )->setFixedWidth( ( *edit )->fontMetrics().horizontalAdvance( QStringLiteral( "000000.000" ) ) + 16 );
        connect( *edit, &QLineEdit::textEdited,
                 &M_ranges, set );
        QPushButton * button = new QPushButton( tr( "Now" ) );
        button->setObjectName( QString( name ) + "_now" );
        button->setToolTip( now_tip );
        button->setAutoDefault( false );
        connect( button, &QPushButton::clicked, this, now );
        // one line, cut by the width of the dialog: the whole text in the
        // tooltip, and the reason of a wrong one in the summary too
        *info = new QLabel();
        ( *info )->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Preferred );
        ( *info )->setMinimumWidth( 120 );
        QHBoxLayout * layout = new QHBoxLayout();
        layout->addWidget( *edit );
        layout->addWidget( button );
        layout->addWidget( *info, 1 );
        form->addRow( label, layout );
    };
    // the shortcuts of the main window that do the same (D7, D12)
    position( tr( "Start:" ), "start", tr( "the first frame" ), &M_start_edit, &M_start_info,
              &ExportRangeList::setStart, &ExportDialog::setStartToCurrent,
              tr( "Take the frame shown now as the start.\n"
                  "In the main window, %1 does it too, with this dialog closed\n"
                  "(File > Export Images / Video > Start the Range Here)." )
              .arg( QKeySequence( Qt::Key_BracketLeft ).toString( QKeySequence::NativeText ) ) );
    position( tr( "End:" ), "end", tr( "the last frame" ), &M_end_edit, &M_end_info,
              &ExportRangeList::setEnd, &ExportDialog::setEndToCurrent,
              tr( "Take the frame shown now as the end.\n"
                  "In the main window, %1 does it too, with this dialog closed\n"
                  "(File > Export Images / Video > End the Range Here)." )
              .arg( QKeySequence( Qt::Key_BracketRight ).toString( QKeySequence::NativeText ) ) );

    // the seconds of the presets (D8)
    {
        const auto seconds = [this]( const char * name ) {
            QDoubleSpinBox * spin = new QDoubleSpinBox();
            spin->setObjectName( name );
            spin->setRange( 0.0, 600.0 );
            spin->setDecimals( 1 );
            spin->setSuffix( tr( " s" ) );
            return spin;
        };
        M_goal_before_spin = seconds( "goal_before" );
        M_goal_after_spin = seconds( "goal_after" );
        M_around_before_spin = seconds( "around_before" );
        M_around_after_spin = seconds( "around_after" );

        QHBoxLayout * row = new QHBoxLayout();
        row->addWidget( new QLabel( tr( "Goal scenes:" ) ) );
        row->addWidget( M_goal_before_spin );
        row->addWidget( new QLabel( tr( "before," ) ) );
        row->addWidget( M_goal_after_spin );
        row->addWidget( new QLabel( tr( "after." ) ) );
        row->addSpacing( 12 );
        row->addWidget( new QLabel( tr( "Around now:" ) ) );
        row->addWidget( M_around_before_spin );
        row->addWidget( new QLabel( tr( "before," ) ) );
        row->addWidget( M_around_after_spin );
        row->addWidget( new QLabel( tr( "after." ) ) );
        row->addStretch( 1 );
        form->addRow( tr( "Presets:" ), row );
    }

    return box;
}

/*-------------------------------------------------------------------*/
void
ExportDialog::createWidgets()
{
    QVBoxLayout * top = new QVBoxLayout( this );

    top->addWidget( createRangeBox() );

    // the images
    {
        QGroupBox * box = new QGroupBox( tr( "Images" ) );
        QFormLayout * form = new QFormLayout( box );

        // the view that draws them: the one shown when the dialog is opened (D17)
        M_view_2d_radio = new QRadioButton( tr( "2D view" ) );
        M_view_2d_radio->setObjectName( "view_2d" );
        M_view_2d_radio->setChecked( true );
        M_view_3d_radio = new QRadioButton( tr( "3D view" ) );
        M_view_3d_radio->setObjectName( "view_3d" );
        QButtonGroup * views = new QButtonGroup( box );
        views->addButton( M_view_2d_radio );
        views->addButton( M_view_3d_radio );
        connect( M_view_3d_radio, &QRadioButton::toggled,
                 this, [this]( bool ) { showView(); } );
        QHBoxLayout * view = new QHBoxLayout();
        view->addWidget( M_view_2d_radio );
        view->addWidget( M_view_3d_radio );
        view->addStretch( 1 );
        form->addRow( tr( "View:" ), view );

        M_size_combo = new QComboBox();
        M_size_combo->setObjectName( "size" );
        M_size_combo->addItem( tr( "As the view" ), SIZES[0].key );
        for ( int i = 1; i < 4; ++i )
        {
            M_size_combo->addItem( QString( "%1 x %2" ).arg( SIZES[i].size.width() ).arg( SIZES[i].size.height() ), SIZES[i].key );
        }
        M_size_combo->addItem( tr( "Custom" ), SIZES[4].key );
        M_width_spin = new QSpinBox();
        M_width_spin->setObjectName( "width" );
        M_width_spin->setRange( 16, 7680 );
        M_height_spin = new QSpinBox();
        M_height_spin->setObjectName( "height" );
        M_height_spin->setRange( 16, 4320 );
        connect( M_size_combo, QOverload< int >::of( &QComboBox::currentIndexChanged ),
                 this, &ExportDialog::updateSummary );
        connect( M_width_spin, QOverload< int >::of( &QSpinBox::valueChanged ),
                 this, &ExportDialog::updateSummary );
        connect( M_height_spin, QOverload< int >::of( &QSpinBox::valueChanged ),
                 this, &ExportDialog::updateSummary );
        QHBoxLayout * size = new QHBoxLayout();
        size->addWidget( M_size_combo );
        size->addWidget( M_width_spin );
        size->addWidget( new QLabel( QStringLiteral( "x" ) ) );
        size->addWidget( M_height_spin );
        size->addStretch( 1 );
        form->addRow( tr( "Size:" ), size );

        M_scale_texts_check = new QCheckBox( tr( "Enlarge the texts, logos and lines with the image" ) );
        M_scale_texts_check->setObjectName( "scale_texts" );
        M_scale_texts_check->setToolTip( tr( "The image looks as the view, enlarged: the texts, the team logos and the widths of the lines grow with the image. "
                                             "Off: they keep their sizes in pixels, smaller and thinner in a large image, and in 2D the players move more smoothly." ) );
        connect( M_scale_texts_check, &QCheckBox::toggled,
                 this, &ExportDialog::updateSummary );
        form->addRow( QString(), M_scale_texts_check );

        M_fps_spin = new QDoubleSpinBox();
        M_fps_spin->setObjectName( "fps" );
        M_fps_spin->setRange( 1.0, 120.0 );
        M_fps_spin->setDecimals( 1 );
        M_fps_spin->setSuffix( tr( " images/s" ) );
        M_speed_spin = new QDoubleSpinBox();
        M_speed_spin->setObjectName( "speed" );
        M_speed_spin->setRange( 0.1, 16.0 );
        M_speed_spin->setDecimals( 2 );
        M_speed_spin->setSingleStep( 0.5 );
        M_speed_spin->setPrefix( tr( "x " ) );
        M_speed_spin->setToolTip( tr( "How many times as fast as the game is played" ) );
        connect( M_fps_spin, QOverload< double >::of( &QDoubleSpinBox::valueChanged ),
                 this, &ExportDialog::updateSummary );
        connect( M_speed_spin, QOverload< double >::of( &QDoubleSpinBox::valueChanged ),
                 this, &ExportDialog::updateSummary );
        QHBoxLayout * rate = new QHBoxLayout();
        rate->addWidget( M_fps_spin );
        rate->addWidget( new QLabel( tr( "Speed:" ) ) );
        rate->addWidget( M_speed_spin );
        M_playback_speed_button = new QPushButton( tr( "As the Playback" ) );
        M_playback_speed_button->setObjectName( "playback_speed" );
        M_playback_speed_button->setToolTip( tr( "The speed of the playback of the monitor: the simulation step / the interval of the playback" ) );
        M_playback_speed_button->setAutoDefault( false );
        connect( M_playback_speed_button, &QPushButton::clicked,
                 this, &ExportDialog::setPlaybackSpeed );
        rate->addWidget( M_playback_speed_button );
        rate->addStretch( 1 );
        form->addRow( tr( "Rate:" ), rate );

        top->addWidget( box );
    }

    // where they go: images, or a video
    {
        QGroupBox * box = new QGroupBox( tr( "Output" ) );
        QVBoxLayout * layout = new QVBoxLayout( box );

        M_images_radio = new QRadioButton( tr( "Images" ) );
        M_images_radio->setObjectName( "output_images" );
        M_video_radio = new QRadioButton( tr( "Video (ffmpeg)" ) );
        M_video_radio->setObjectName( "output_video" );
        connect( M_video_radio, &QRadioButton::toggled,
                 this, &ExportDialog::showOutput );
        // exclusive in pairs, not with the radio buttons of the ranges
        QButtonGroup * kind_group = new QButtonGroup( box );
        kind_group->addButton( M_images_radio );
        kind_group->addButton( M_video_radio );
        QHBoxLayout * kind = new QHBoxLayout();
        kind->addWidget( M_images_radio );
        kind->addWidget( M_video_radio );
        kind->addStretch( 1 );
        layout->addLayout( kind );

        // the images
        M_images_widget = new QWidget();
        {
            QFormLayout * form = new QFormLayout( M_images_widget );
            form->setContentsMargins( 0, 0, 0, 0 );

            M_dir_edit = new QLineEdit();
            M_dir_edit->setObjectName( "directory" );
            connect( M_dir_edit, &QLineEdit::textChanged,
                     this, &ExportDialog::updateSummary );
            QPushButton * browse = new QPushButton( tr( "Browse..." ) );
            browse->setAutoDefault( false );
            connect( browse, &QPushButton::clicked,
                     this, &ExportDialog::chooseDirectory );
            QHBoxLayout * dir = new QHBoxLayout();
            dir->addWidget( M_dir_edit, 1 );
            dir->addWidget( browse );
            form->addRow( tr( "Directory:" ), dir );

            M_prefix_edit = new QLineEdit();
            M_prefix_edit->setObjectName( "prefix" );
            M_prefix_edit->setMaximumWidth( 160 );
            connect( M_prefix_edit, &QLineEdit::textChanged,
                     this, &ExportDialog::updateSummary );
            M_format_combo = new QComboBox();
            M_format_combo->setObjectName( "format" );
            for ( const QByteArray & f : ImageSequenceSink::formats() )
            {
                M_format_combo->addItem( QString::fromLatin1( f ).toUpper(), QString::fromLatin1( f ) );
            }
            connect( M_format_combo, QOverload< int >::of( &QComboBox::currentIndexChanged ),
                     this, &ExportDialog::updateSummary );
            QHBoxLayout * name = new QHBoxLayout();
            name->addWidget( M_prefix_edit );
            name->addWidget( M_format_combo );
            name->addStretch( 1 );
            form->addRow( tr( "Names:" ), name );

            M_name_by_cycle_check = new QCheckBox( tr( "Name the images by the cycles" ) );
            M_name_by_cycle_check->setObjectName( "name_by_cycle" );
            M_name_by_cycle_check->setToolTip( tr( "e.g. image-c01234.00.png: when an image is a frame (10 images/s at x 1)" ) );
            connect( M_name_by_cycle_check, &QCheckBox::toggled,
                     this, &ExportDialog::updateSummary );
            form->addRow( QString(), M_name_by_cycle_check );
        }
        layout->addWidget( M_images_widget );

        // the video (D14, D15)
        M_video_widget = new QWidget();
        {
            QFormLayout * form = new QFormLayout( M_video_widget );
            form->setContentsMargins( 0, 0, 0, 0 );

            M_video_edit = new QLineEdit();
            M_video_edit->setObjectName( "video_file" );
            M_video_edit->setToolTip( tr( "The format is by the extension: .mp4, .mov or .mkv (.mp4 when there is none)." ) );
            connect( M_video_edit, &QLineEdit::textChanged,
                     this, &ExportDialog::updateSummary );
            QPushButton * browse = new QPushButton( tr( "Browse..." ) );
            browse->setAutoDefault( false );
            connect( browse, &QPushButton::clicked,
                     this, &ExportDialog::chooseVideoFile );
            QHBoxLayout * file = new QHBoxLayout();
            file->addWidget( M_video_edit, 1 );
            file->addWidget( browse );
            form->addRow( tr( "File:" ), file );

            M_crf_spin = new QSpinBox();
            M_crf_spin->setObjectName( "crf" );
            M_crf_spin->setRange( 0, 51 );
            M_crf_spin->setToolTip( tr( "The CRF of H.264: smaller is better and larger (18: hardly any loss to see; 23: the default of ffmpeg)." ) );
            M_preset_combo = new QComboBox();
            M_preset_combo->setObjectName( "preset" );
            M_preset_combo->addItems( FfmpegSink::PRESETS );
            M_preset_combo->setToolTip( tr( "The speed of the compression: slower makes smaller files of the same quality." ) );
            QHBoxLayout * quality = new QHBoxLayout();
            quality->addWidget( M_crf_spin );
            quality->addWidget( new QLabel( tr( "Encoding speed:" ) ) );
            quality->addWidget( M_preset_combo );
            quality->addStretch( 1 );
            form->addRow( tr( "Quality (CRF):" ), quality );

            M_ffmpeg_edit = new QLineEdit();
            M_ffmpeg_edit->setObjectName( "ffmpeg" );
            M_ffmpeg_edit->setPlaceholderText( tr( "ffmpeg in the PATH" ) );
            connect( M_ffmpeg_edit, &QLineEdit::editingFinished,
                     this, &ExportDialog::probeFfmpeg );
            QPushButton * find = new QPushButton( tr( "Browse..." ) );
            find->setAutoDefault( false );
            connect( find, &QPushButton::clicked,
                     this, &ExportDialog::chooseFfmpeg );
            QHBoxLayout * program = new QHBoxLayout();
            program->addWidget( M_ffmpeg_edit, 1 );
            program->addWidget( find );
            form->addRow( tr( "ffmpeg:" ), program );

            M_ffmpeg_label = new QLabel();
            M_ffmpeg_label->setObjectName( "ffmpeg_status" );
            M_ffmpeg_label->setWordWrap( true );
            form->addRow( QString(), M_ffmpeg_label );
        }
        layout->addWidget( M_video_widget );

        // several ranges
        M_separate_radio = new QRadioButton();
        M_separate_radio->setObjectName( "separate_ranges" );
        M_one_sequence_radio = new QRadioButton();
        M_one_sequence_radio->setObjectName( "one_sequence" );
        connect( M_separate_radio, &QRadioButton::toggled,
                 this, &ExportDialog::updateSummary );
        QButtonGroup * ranges_group = new QButtonGroup( box );
        ranges_group->addButton( M_separate_radio );
        ranges_group->addButton( M_one_sequence_radio );
        QHBoxLayout * ranges = new QHBoxLayout();
        ranges->addWidget( new QLabel( tr( "Ranges:" ) ) );
        ranges->addWidget( M_separate_radio );
        ranges->addWidget( M_one_sequence_radio );
        ranges->addStretch( 1 );
        layout->addLayout( ranges );

        top->addWidget( box );
    }

    M_summary_label = new QLabel();
    M_summary_label->setObjectName( "summary" );
    M_summary_label->setWordWrap( true );
    top->addWidget( M_summary_label );

    M_message_label = new QLabel();
    M_message_label->setObjectName( "message" );
    M_message_label->setWordWrap( true );
    top->addWidget( M_message_label );

    QDialogButtonBox * buttons = new QDialogButtonBox();
    M_export_button = buttons->addButton( tr( "Export" ), QDialogButtonBox::ActionRole );
    M_export_button->setObjectName( "export" );
    M_export_button->setAutoDefault( false );
    connect( M_export_button, &QPushButton::clicked,
             this, [this]() { startExport( true ); } );
    QPushButton * close = buttons->addButton( QDialogButtonBox::Close );
    close->setAutoDefault( false );
    connect( close, &QPushButton::clicked,
             this, &QDialog::close );
    top->addWidget( buttons );
}

/*-------------------------------------------------------------------*/
void
ExportDialog::readSettings()
{
    const Options::ExportSettings & s = Options::instance().exportSettings();

    const int size = M_size_combo->findData( s.size );
    M_size_combo->setCurrentIndex( size >= 0 ? size : 0 );
    M_width_spin->setValue( s.width );
    M_height_spin->setValue( s.height );
    M_scale_texts_check->setChecked( s.scale_texts );
    // the fps of the view chosen in the field, the other's kept
    M_fps_spin->setValue( isView3D() ? s.fps_3d : s.fps );
    M_other_fps = ( isView3D() ? s.fps : s.fps_3d );
    M_speed_spin->setValue( s.speed );
    M_dir_edit->setText( s.directory.isEmpty() ? QDir::homePath() : s.directory );
    M_prefix_edit->setText( s.prefix );
    const int format = M_format_combo->findData( s.format );
    M_format_combo->setCurrentIndex( format >= 0 ? format : 0 );
    M_name_by_cycle_check->setChecked( s.name_by_cycle );
    ( s.separate_ranges ? M_separate_radio : M_one_sequence_radio )->setChecked( true );
    M_goal_before_spin->setValue( s.goal_before );
    M_goal_after_spin->setValue( s.goal_after );
    M_around_before_spin->setValue( s.around_before );
    M_around_after_spin->setValue( s.around_after );
    M_video_edit->setText( s.video_file.isEmpty()
                           ? QDir( QDir::homePath() ).filePath( QStringLiteral( "rcssmonitor.mp4" ) )
                           : s.video_file );
    M_crf_spin->setValue( s.crf );
    const int preset = M_preset_combo->findText( s.preset );
    M_preset_combo->setCurrentIndex( preset >= 0 ? preset : M_preset_combo->findText( QStringLiteral( "medium" ) ) );
    M_ffmpeg_edit->setText( s.ffmpeg );
    ( s.video ? M_video_radio : M_images_radio )->setChecked( true );
    showOutput();
}

/*-------------------------------------------------------------------*/
void
ExportDialog::writeSettings() const
{
    Options::ExportSettings s;
    s.directory = directory();
    s.prefix = M_prefix_edit->text();
    s.format = M_format_combo->currentData().toString();
    s.size = M_size_combo->currentData().toString();
    s.width = M_width_spin->value();
    s.height = M_height_spin->value();
    s.scale_texts = M_scale_texts_check->isChecked();
    s.fps = ( isView3D() ? M_other_fps : M_fps_spin->value() );
    s.fps_3d = ( isView3D() ? M_fps_spin->value() : M_other_fps );
    s.speed = M_speed_spin->value();
    s.name_by_cycle = M_name_by_cycle_check->isChecked();
    s.separate_ranges = M_separate_radio->isChecked();
    s.goal_before = M_goal_before_spin->value();
    s.goal_after = M_goal_after_spin->value();
    s.around_before = M_around_before_spin->value();
    s.around_after = M_around_after_spin->value();
    s.video = isVideo();
    s.video_file = M_video_edit->text().trimmed();
    s.crf = M_crf_spin->value();
    s.preset = M_preset_combo->currentText();
    s.ffmpeg = M_ffmpeg_edit->text().trimmed();
    Options::instance().setExportSettings( s );
    Options::instance().writeExportSettings();
}

/*-------------------------------------------------------------------*/
ExportRange
ExportDialog::range( QString * error ) const
{
    return ExportRangeList::resolve( M_ranges.currentRange(), M_disp_holder.dispCont(), error );
}

/*-------------------------------------------------------------------*/
std::vector< ExportRange >
ExportDialog::ranges( QString * error ) const
{
    std::vector< ExportRange > list;
    if ( error ) error->clear();
    for ( const ExportRangeSpec & spec : M_ranges.ranges() )
    {
        QString e;
        const ExportRange r = ExportRangeList::resolve( spec, M_disp_holder.dispCont(), &e );
        if ( ! r.isValid() )
        {
            if ( error ) *error = ( M_ranges.size() > 1 ? spec.name + QStringLiteral( ": " ) + e : e );
            return std::vector< ExportRange >();
        }
        list.push_back( r );
    }
    return list;
}

/*-------------------------------------------------------------------*/
QStringList
ExportDialog::allImageNames( const std::vector< std::vector< std::size_t > > & frames,
                             QString * error ) const
{
    if ( error ) error->clear();
    if ( frames.size() == 1 )
    {
        return imageNames( frames.front() );
    }

    QStringList names;
    if ( M_separate_radio->isChecked() )
    {
        // each range in its own directory, by its name
        const QStringList dirs = rangeFileNames( static_cast< int >( frames.size() ) );
        for ( std::size_t r = 0; r < frames.size(); ++r )
        {
            for ( const QString & name : imageNames( frames[r] ) )
            {
                names << dirs[static_cast< int >( r )] + QLatin1Char( '/' ) + name;
            }
        }
        return names;
    }

    // one sequence: the numbers go on over the ranges
    std::vector< std::size_t > all;
    for ( const std::vector< std::size_t > & f : frames ) all.insert( all.end(), f.begin(), f.end() );
    names = imageNames( all );
    if ( std::set< QString >( names.begin(), names.end() ).size() != static_cast< std::size_t >( names.size() ) )
    {
        if ( error ) *error = tr( "The ranges have the same frames: the names by the cycles repeat in one sequence." );
        return QStringList();
    }
    return names;
}

/*-------------------------------------------------------------------*/
QStringList
ExportDialog::rangeFileNames( const int count ) const
{
    QStringList names;
    std::set< QString > used;
    for ( int r = 0; r < count && r < M_ranges.size(); ++r )
    {
        const QString base = ExportRangeList::safeName( M_ranges.ranges()[r].name );
        QString name = base;
        for ( int n = 2; used.count( name ) > 0; ++n )
        {
            name = base + QString( "-%1" ).arg( n );
        }
        used.insert( name );
        names << name;
    }
    return names;
}

/*-------------------------------------------------------------------*/
std::vector< ExportPeriod >
ExportDialog::periods() const
{
    return ExportRange::periods( M_disp_holder.dispCont(), M_disp_holder.serverParam() );
}

/*-------------------------------------------------------------------*/
std::vector< ExportGoal >
ExportDialog::goals() const
{
    return ExportRange::goals( M_disp_holder.dispCont(), M_disp_holder.scoreChangedIndex() );
}

/*-------------------------------------------------------------------*/
ExportRangeSpec
ExportDialog::specOf( const ExportRange & range,
                      const QString & name ) const
{
    const DispCont & frames = M_disp_holder.dispCont();
    return ExportRangeSpec{ ExportRange::positionText( frames, range.first ),
                            ExportRange::positionText( frames, range.last ),
                            name };
}

/*-------------------------------------------------------------------*/
QString
ExportDialog::rangeText( const ExportRangeSpec & spec ) const
{
    QString error;
    const ExportRange r = ExportRangeList::resolve( spec, M_disp_holder.dispCont(), &error );
    const QString start = spec.start.trimmed().isEmpty() ? tr( "first" ) : spec.start.trimmed();
    const QString end = spec.end.trimmed().isEmpty() ? tr( "last" ) : spec.end.trimmed();
    QString text = QString( "%1: %2 - %3" ).arg( spec.name, start, end );
    if ( r.isValid() )
    {
        text += ( r.size() == 1 ? tr( " (1 frame)" ) : tr( " (%1 frames)" ).arg( r.size() ) );
    }
    else if ( ! M_disp_holder.dispCont().empty() )
    {
        text += QStringLiteral( " (" ) + error + QLatin1Char( ')' );
    }
    return text;
}

/*-------------------------------------------------------------------*/
void
ExportDialog::setStart( const QString & text )
{
    M_ranges.setStart( text );
}

/*-------------------------------------------------------------------*/
void
ExportDialog::setEnd( const QString & text )
{
    M_ranges.setEnd( text );
}

/*-------------------------------------------------------------------*/
void
ExportDialog::setStartToCurrent()
{
    const DispConstPtr disp = M_disp_holder.currentDisp();
    if ( disp ) M_ranges.setStart( ExportRange::positionText( disp->show_ ) );
}

/*-------------------------------------------------------------------*/
void
ExportDialog::setEndToCurrent()
{
    const DispConstPtr disp = M_disp_holder.currentDisp();
    if ( disp ) M_ranges.setEnd( ExportRange::positionText( disp->show_ ) );
}

/*-------------------------------------------------------------------*/
void
ExportDialog::clearRange()
{
    ExportRangeSpec spec = M_ranges.currentRange();
    spec.start.clear();
    spec.end.clear();
    M_ranges.setCurrentRange( spec );
}

/*-------------------------------------------------------------------*/
void
ExportDialog::choosePeriod( const int index )
{
    const std::vector< ExportPeriod > list = periods();
    if ( index < 0 || static_cast< int >( list.size() ) <= index )
    {
        return;
    }
    ExportRange r;
    r.first = list[index].first;
    r.last = list[index].last;
    M_ranges.setCurrentRange( specOf( r, list[index].key ) );
}

/*-------------------------------------------------------------------*/
void
ExportDialog::chooseGoal( const int index )
{
    const std::vector< ExportGoal > list = goals();
    if ( index < 0 || static_cast< int >( list.size() ) <= index )
    {
        return;
    }
    const int step = std::max( 1, M_disp_holder.serverParam().simulator_step_ );
    const ExportRange r = ExportRange::around( M_disp_holder.dispCont(), list[index].cycle,
                                               M_goal_before_spin->value(), M_goal_after_spin->value(), step );
    if ( r.isValid() )
    {
        M_ranges.setCurrentRange( specOf( r, QString( "goal%1-c%2" ).arg( index + 1 ).arg( list[index].cycle ) ) );
    }
}

/*-------------------------------------------------------------------*/
void
ExportDialog::chooseAllGoals()
{
    const std::vector< ExportGoal > list = goals();
    if ( list.empty() )
    {
        return;
    }
    const int step = std::max( 1, M_disp_holder.serverParam().simulator_step_ );
    QVector< ExportRangeSpec > specs;
    // after the ranges given, or in place of the whole log
    const bool whole = ( M_ranges.size() == 1
                         && M_ranges.currentRange().start.trimmed().isEmpty()
                         && M_ranges.currentRange().end.trimmed().isEmpty() );
    if ( ! whole )
    {
        specs = M_ranges.ranges();
    }
    for ( std::size_t i = 0; i < list.size(); ++i )
    {
        const ExportRange r = ExportRange::around( M_disp_holder.dispCont(), list[i].cycle,
                                                   M_goal_before_spin->value(), M_goal_after_spin->value(), step );
        if ( r.isValid() )
        {
            specs.push_back( specOf( r, QString( "goal%1-c%2" ).arg( i + 1 ).arg( list[i].cycle ) ) );
        }
    }
    M_ranges.setRanges( specs );
}

/*-------------------------------------------------------------------*/
void
ExportDialog::chooseAround()
{
    const DispConstPtr disp = M_disp_holder.currentDisp();
    if ( ! disp )
    {
        return;
    }
    const int step = std::max( 1, M_disp_holder.serverParam().simulator_step_ );
    const int cycle = static_cast< int >( disp->show_.time_ );
    const ExportRange r = ExportRange::around( M_disp_holder.dispCont(), cycle,
                                               M_around_before_spin->value(), M_around_after_spin->value(), step );
    if ( r.isValid() )
    {
        M_ranges.setCurrentRange( specOf( r, QString( "around-c%1" ).arg( cycle ) ) );
    }
}

/*-------------------------------------------------------------------*/
void
ExportDialog::addRange()
{
    M_ranges.add( ExportRangeSpec{ QString(), QString(), M_ranges.newName() } );
}

/*-------------------------------------------------------------------*/
void
ExportDialog::removeRange()
{
    M_ranges.remove( M_ranges.current() );
}

/*-------------------------------------------------------------------*/
void
ExportDialog::moveRangeUp()
{
    M_ranges.move( M_ranges.current(), M_ranges.current() - 1 );
}

/*-------------------------------------------------------------------*/
void
ExportDialog::moveRangeDown()
{
    M_ranges.move( M_ranges.current(), M_ranges.current() + 1 );
}

/*-------------------------------------------------------------------*/
/*!
  The presets of the log open: the whole log, the parts of the game, the
  goals (each, and all of them as ranges), and around the position shown.
*/
void
ExportDialog::buildPresetsMenu()
{
    M_presets_menu->clear();
    const DispCont & frames = M_disp_holder.dispCont();
    if ( frames.empty() )
    {
        M_presets_menu->addAction( tr( "There is no game log." ) )->setEnabled( false );
        return;
    }
    const int step = std::max( 1, M_disp_holder.serverParam().simulator_step_ );

    QAction * all = M_presets_menu->addAction( tr( "The whole log" ) );
    connect( all, &QAction::triggered, this, &ExportDialog::clearRange );

    M_presets_menu->addSeparator();
    const std::vector< ExportPeriod > parts = periods();
    for ( std::size_t i = 0; i < parts.size(); ++i )
    {
        QAction * act = M_presets_menu->addAction( QString( "%1 (%2 - %3)" )
                                                   .arg( parts[i].label,
                                                         ExportRange::gameTime( frames[parts[i].first]->show_.time_, step ),
                                                         ExportRange::gameTime( frames[parts[i].last]->show_.time_, step ) ) );
        act->setObjectName( QStringLiteral( "preset_" ) + parts[i].key );
        const int index = static_cast< int >( i );
        connect( act, &QAction::triggered, this, [this, index]() { choosePeriod( index ); } );
    }

    M_presets_menu->addSeparator();
    const std::vector< ExportGoal > list = goals();
    if ( list.empty() )
    {
        M_presets_menu->addAction( tr( "No goals" ) )->setEnabled( false );
    }
    for ( std::size_t i = 0; i < list.size(); ++i )
    {
        QAction * act = M_presets_menu->addAction( tr( "Goal %1: %2 %3 (%4 - %5)" )
                                                   .arg( i + 1 )
                                                   .arg( ExportRange::gameTime( list[i].cycle, step ), list[i].team )
                                                   .arg( list[i].left ).arg( list[i].right ) );
        act->setObjectName( QString( "preset_goal%1" ).arg( i + 1 ) );
        const int index = static_cast< int >( i );
        connect( act, &QAction::triggered, this, [this, index]() { chooseGoal( index ); } );
    }
    if ( ! list.empty() )
    {
        QAction * every = M_presets_menu->addAction( tr( "All the goals, a range each" ) );
        every->setObjectName( "preset_all_goals" );
        connect( every, &QAction::triggered, this, &ExportDialog::chooseAllGoals );
    }

    M_presets_menu->addSeparator();
    QAction * around = M_presets_menu->addAction( tr( "Around the position shown" ) );
    around->setObjectName( "preset_around" );
    connect( around, &QAction::triggered, this, &ExportDialog::chooseAround );
}

/*-------------------------------------------------------------------*/
void
ExportDialog::showRanges()
{
    // the list
    {
        const QSignalBlocker block( M_range_list );
        M_range_list->clear();
        for ( const ExportRangeSpec & spec : M_ranges.ranges() )
        {
            M_range_list->addItem( rangeText( spec ) );
        }
        M_range_list->setCurrentRow( M_ranges.current() );
    }

    // the fields of the current range: only when they differ, not to move
    // the cursor of a field being edited
    const ExportRangeSpec & spec = M_ranges.currentRange();
    if ( M_name_edit->text() != spec.name ) M_name_edit->setText( spec.name );
    if ( M_start_edit->text() != spec.start ) M_start_edit->setText( spec.start );
    if ( M_end_edit->text() != spec.end ) M_end_edit->setText( spec.end );

    M_up_button->setEnabled( M_ranges.current() > 0 );
    M_down_button->setEnabled( M_ranges.current() + 1 < M_ranges.size() );
    M_remove_button->setEnabled( M_ranges.size() > 1 || ! M_ranges.isCleared() );
    M_separate_radio->setEnabled( M_ranges.size() > 1 );
    M_one_sequence_radio->setEnabled( M_ranges.size() > 1 );

    updateSummary();
}
/*-------------------------------------------------------------------*/
double
ExportDialog::framesPerImage() const
{
    return ExportTiming::framesPerImage( M_speed_spin->value(),
                                         std::max( 1, M_disp_holder.serverParam().simulator_step_ ),
                                         M_fps_spin->value() );
}

/*-------------------------------------------------------------------*/
std::vector< std::size_t >
ExportDialog::imageFrames( const ExportRange & range ) const
{
    if ( ! range.isValid() )
    {
        return std::vector< std::size_t >();
    }
    return ExportTiming::frames2D( range.first, range.last, framesPerImage() );
}

/*-------------------------------------------------------------------*/
std::vector< double >
ExportDialog::imagePositions( const ExportRange & range ) const
{
    if ( ! range.isValid() )
    {
        return std::vector< double >();
    }
    return ExportTiming::positions3D( range.first, range.last, framesPerImage() );
}

/*-------------------------------------------------------------------*/
bool
ExportDialog::isView3D() const
{
    return ( M_view_3d_radio->isChecked()
             && M_offscreen_3d
             && M_view_3d_reason.isEmpty() );
}

/*-------------------------------------------------------------------*/
void
ExportDialog::setView3DSources( const FieldView3D * view,
                                OffscreenView3D * offscreen )
{
    M_view_3d = view;
#ifdef HAVE_3D_VIEW
    if ( ! offscreen )
    {
        M_view_3d_reason = tr( "The 3D view is not available." );
    }
    else if ( offscreen != M_offscreen_3d )
    {
        // a context is tried once for each drawer
        M_view_3d_reason = OffscreenView3D::unavailableReason();
    }
#else
    M_view_3d_reason = tr( "This rcssmonitor is built without the 3D view." );
#endif
    M_offscreen_3d = offscreen;

    const bool available = M_view_3d_reason.isEmpty();
    M_view_3d_radio->setEnabled( available );
    M_view_3d_radio->setToolTip( available
                                 ? tr( "The 3D view drawn between the frames, the camera starting as the view's" )
                                 : tr( "Not available: %1" ).arg( M_view_3d_reason ) );
    if ( ! available
         && M_view_3d_radio->isChecked() )
    {
        M_view_2d_radio->setChecked( true );
    }
    updateSummary();
}

/*-------------------------------------------------------------------*/
void
ExportDialog::chooseView3D( const bool on )
{
    ( on && M_view_3d_radio->isEnabled()
      ? M_view_3d_radio
      : M_view_2d_radio )->setChecked( true );
}

/*-------------------------------------------------------------------*/
void
ExportDialog::showView()
{
    // the fps of each view: 10 for the frames of 2D, 30 for 3D by default
    const double shown = M_fps_spin->value();
    M_fps_spin->setValue( M_other_fps );
    M_other_fps = shown;
    updateSummary();
}

/*-------------------------------------------------------------------*/
QSize
ExportDialog::viewPixels() const
{
#ifdef HAVE_3D_VIEW
    if ( M_view_3d
         && M_view_3d->isExposed() )
    {
        const qreal dpr = M_view_3d->devicePixelRatio();
        return QSize( std::max( 1, static_cast< int >( std::lround( M_view_3d->width() * dpr ) ) ),
                      std::max( 1, static_cast< int >( std::lround( M_view_3d->height() * dpr ) ) ) );
    }
#endif
    const qreal dpr = M_canvas.devicePixelRatioF();
    return QSize( std::max( 1, static_cast< int >( std::lround( M_canvas.width() * dpr ) ) ),
                  std::max( 1, static_cast< int >( std::lround( M_canvas.height() * dpr ) ) ) );
}

/*-------------------------------------------------------------------*/
int
ExportDialog::viewHeight() const
{
#ifdef HAVE_3D_VIEW
    if ( M_view_3d
         && M_view_3d->isExposed() )
    {
        return M_view_3d->height();
    }
#endif
    return M_canvas.height();
}

/*-------------------------------------------------------------------*/
QSize
ExportDialog::imageSize() const
{
    const QString key = M_size_combo->currentData().toString();
    QSize size( M_width_spin->value(), M_height_spin->value() );
    if ( key == QLatin1String( "view" ) )
    {
        // the view shown: the 2D view or the 3D view
        size = viewPixels();
    }
    for ( const SizeChoice & c : SIZES )
    {
        if ( key == QLatin1String( c.key ) && c.size.isValid() ) size = c.size;
    }
    if ( isVideo() )
    {
        // the colors of a video are of 2 x 2 pixels (yuv420p)
        size = QSize( std::max( 2, size.width() & ~1 ),
                      std::max( 2, size.height() & ~1 ) );
    }
    return size;
}

/*-------------------------------------------------------------------*/
qreal
ExportDialog::imageRatio() const
{
    // the texts, the logos and the lines as large as in the view: the image is the view's layout,
    // scaled to the height of the image
    const int height = viewHeight();
    if ( ! M_scale_texts_check->isChecked()
         || height <= 0 )
    {
        return 1.0;
    }
    return std::max( 0.1, static_cast< qreal >( imageSize().height() ) / height );
}

/*-------------------------------------------------------------------*/
bool
ExportDialog::canNameByCycle() const
{
    // one frame an image: no frame twice, no frame skipped
    return std::fabs( framesPerImage() - 1.0 ) < ExportTiming::EPSILON;
}

/*-------------------------------------------------------------------*/
QStringList
ExportDialog::imageNames( const std::vector< std::size_t > & frames ) const
{
    QStringList names;
    const QString prefix = M_prefix_edit->text();
    const QString ext = M_format_combo->currentData().toString();

    if ( M_name_by_cycle_check->isChecked()
         && canNameByCycle() )
    {
        // PREFIXc01234.03.png: the digits of the largest cycle and stopped frame
        const DispCont & cont = M_disp_holder.dispCont();
        unsigned long long max_cycle = 0;
        unsigned long long max_stopped = 0;
        for ( const std::size_t i : frames )
        {
            max_cycle = std::max< unsigned long long >( max_cycle, cont[i]->show_.time_ );
            max_stopped = std::max< unsigned long long >( max_stopped, cont[i]->show_.stime_ );
        }
        const int cycle_digits = std::max( 5, digitsOf( max_cycle ) );
        const int stopped_digits = std::max( 2, digitsOf( max_stopped ) );
        for ( const std::size_t i : frames )
        {
            names << prefix
                + QString( "c%1.%2." ).arg( cont[i]->show_.time_, cycle_digits, 10, QLatin1Char( '0' ) )
                                      .arg( cont[i]->show_.stime_, stopped_digits, 10, QLatin1Char( '0' ) )
                + ext;
        }
        return names;
    }

    const int digits = ExportTiming::nameDigits( frames.size() );
    for ( std::size_t n = 0; n < frames.size(); ++n )
    {
        names << ExportTiming::imageName( prefix, n, digits, ext );
    }
    return names;
}

/*-------------------------------------------------------------------*/
QString
ExportDialog::directory() const
{
    const QString dir = M_dir_edit->text().trimmed();
    return dir.isEmpty() ? QDir::homePath() : QDir::cleanPath( dir );
}

/*-------------------------------------------------------------------*/
bool
ExportDialog::isVideo() const
{
    return M_video_radio->isChecked();
}

/*-------------------------------------------------------------------*/
QString
ExportDialog::videoFile() const
{
    QString file = M_video_edit->text().trimmed();
    if ( file.isEmpty() )
    {
        file = QDir( QDir::homePath() ).filePath( QStringLiteral( "rcssmonitor.mp4" ) );
    }
    file = QDir::cleanPath( file );
    if ( QFileInfo( file ).suffix().isEmpty() )
    {
        file += QStringLiteral( ".mp4" );
    }
    return file;
}

/*-------------------------------------------------------------------*/
QStringList
ExportDialog::videoFiles( const int ranges ) const
{
    const QString file = videoFile();
    if ( ranges <= 1
         || ! M_separate_radio->isChecked() )
    {
        return QStringList( file );
    }

    // FILE-NAME.ext: a video for each range, by its name
    const QFileInfo info( file );
    const QString base = QDir::cleanPath( QDir( info.path() ).filePath( info.completeBaseName() ) );
    QStringList files;
    for ( const QString & name : rangeFileNames( ranges ) )
    {
        files << base + QLatin1Char( '-' ) + name + QLatin1Char( '.' ) + info.suffix();
    }
    return files;
}

/*-------------------------------------------------------------------*/
QString
ExportDialog::summaryText() const
{
    return M_summary_label->text();
}

/*-------------------------------------------------------------------*/
QString
ExportDialog::messageText() const
{
    return M_message_label->text();
}

/*-------------------------------------------------------------------*/
bool
ExportDialog::isExporting() const
{
    return M_job && M_job->isRunning();
}

/*-------------------------------------------------------------------*/
void
ExportDialog::setDirectory( const QString & dir )
{
    M_dir_edit->setText( dir );
}

/*-------------------------------------------------------------------*/
void
ExportDialog::setVideo( const bool video )
{
    ( video ? M_video_radio : M_images_radio )->setChecked( true );
}

/*-------------------------------------------------------------------*/
void
ExportDialog::setVideoFile( const QString & file )
{
    M_video_edit->setText( file );
}

/*-------------------------------------------------------------------*/
void
ExportDialog::setFfmpeg( const QString & path )
{
    M_ffmpeg_edit->setText( path );
    probeFfmpeg();
}

/*-------------------------------------------------------------------*/
void
ExportDialog::probeFfmpeg()
{
    M_ffmpeg_info = FfmpegSink::probe( M_ffmpeg_edit->text() );
    M_ffmpeg_probed = true;

    QString text;
    if ( ! M_ffmpeg_info.found )
    {
        text = M_ffmpeg_info.error + QLatin1Char( ' ' )
            + tr( "A video needs ffmpeg: install it, or choose it." );
    }
    else
    {
        // "ffmpeg version 6.1.1 Copyright (c) ...": its version
        const QString version = M_ffmpeg_info.version.section( QStringLiteral( " Copyright" ), 0, 0 );
        text = tr( "%1; H.264 by %2." ).arg( version, M_ffmpeg_info.encoder );
        if ( M_ffmpeg_info.encoder == QLatin1String( "mpeg4" ) )
        {
            text = tr( "%1; no H.264: MPEG-4 Part 2 by mpeg4, larger and of lower quality." ).arg( version );
        }
        if ( M_ffmpeg_info.encoder != QLatin1String( "libx264" ) )
        {
            text += QLatin1Char( ' ' ) + tr( "The quality and the speed are of libx264, not used." );
        }
    }
    M_ffmpeg_label->setText( text );
    M_ffmpeg_label->setToolTip( QDir::toNativeSeparators( M_ffmpeg_info.program ) );
    UIPalette::markWrongText( M_ffmpeg_label, ! M_ffmpeg_info.found );

    const bool x264 = ( ! M_ffmpeg_info.found || M_ffmpeg_info.encoder == QLatin1String( "libx264" ) );
    M_crf_spin->setEnabled( x264 );
    M_preset_combo->setEnabled( x264 );
    updateSummary();
}

/*-------------------------------------------------------------------*/
void
ExportDialog::setPlaybackSpeed()
{
    // the monitor shows a frame each interval (ms), and a cycle is a step (ms)
    const int step = std::max( 1, M_disp_holder.serverParam().simulator_step_ );
    const int interval = std::max( 1, Options::instance().timerInterval() );
    M_speed_spin->setValue( static_cast< double >( step ) / interval );
}

/*-------------------------------------------------------------------*/
void
ExportDialog::setExportAllowed( const bool allowed )
{
    M_allowed = allowed;
    updateSummary();
}

/*-------------------------------------------------------------------*/
void
ExportDialog::chooseDirectory()
{
    const QString dir = QFileDialog::getExistingDirectory( this,
                                                           tr( "The directory of the images" ),
                                                           directory() );
    if ( ! dir.isEmpty() )
    {
        M_dir_edit->setText( dir );
    }
}

/*-------------------------------------------------------------------*/
void
ExportDialog::chooseVideoFile()
{
    // the files there are asked about at the export
    const QString file = QFileDialog::getSaveFileName( this,
                                                       tr( "The video file" ),
                                                       videoFile(),
                                                       tr( "Videos (*.mp4 *.mov *.mkv)" ),
                                                       nullptr,
                                                       QFileDialog::DontConfirmOverwrite );
    if ( ! file.isEmpty() )
    {
        M_video_edit->setText( file );
    }
}

/*-------------------------------------------------------------------*/
void
ExportDialog::chooseFfmpeg()
{
    const QString program = FfmpegSink::findProgram( M_ffmpeg_edit->text() );
    const QString file = QFileDialog::getOpenFileName( this,
                                                       tr( "ffmpeg" ),
                                                       program.isEmpty() ? QDir::homePath() : program );
    if ( ! file.isEmpty() )
    {
        setFfmpeg( file );
    }
}

/*-------------------------------------------------------------------*/
void
ExportDialog::showOutput()
{
    const bool video = isVideo();
    M_images_widget->setVisible( ! video );
    M_video_widget->setVisible( video );
    M_separate_radio->setText( video
                               ? tr( "A video for each, by its name" )
                               : tr( "Each in its own directory, by its name" ) );
    M_one_sequence_radio->setText( video
                                   ? tr( "All in one video" )
                                   : tr( "All in one sequence" ) );
    if ( video
         && ! M_ffmpeg_probed )
    {
        probeFfmpeg();
    }
    updateSummary();
}

/*-------------------------------------------------------------------*/
void
ExportDialog::showMessage( const QString & text,
                           const bool error )
{
    M_message_label->setText( text );
    UIPalette::markWrongText( M_message_label, error );
}

/*-------------------------------------------------------------------*/
void
ExportDialog::abortExport()
{
    if ( M_job )
    {
        M_job->abort();
    }
}

/*-------------------------------------------------------------------*/
void
ExportDialog::handleFinished( bool ok,
                              const QString & message )
{
    if ( M_progress )
    {
        M_progress->hide();
        M_progress->deleteLater();
        M_progress = nullptr;
    }
    if ( M_job )
    {
        M_job->deleteLater();
        M_job = nullptr;
    }

    // the videos: one, or several in a directory
    QString what = message;
    const QStringList lines = message.split( QLatin1Char( '\n' ), Qt::SkipEmptyParts );
    if ( ok && M_job_video )
    {
        what = ( lines.size() == 1
                 ? tr( "the video %1" ).arg( lines.front() )
                 : tr( "%1 videos in %2" ).arg( lines.size() )
                 .arg( QDir::toNativeSeparators( QFileInfo( QDir::fromNativeSeparators( lines.front() ) ).absolutePath() ) ) );
    }
    else if ( ok && lines.size() > 1 )
    {
        what = lines.join( QStringLiteral( "; " ) );
    }
    showMessage( ok ? tr( "Exported %1." ).arg( what ) : message, ! ok );
    updateSummary();
    emit exportFinished( ok, what );
}
/*-------------------------------------------------------------------*/
void
ExportDialog::updateSummary()
{
    const DispCont & frames = M_disp_holder.dispCont();
    const int step = std::max( 1, M_disp_holder.serverParam().simulator_step_ );

    // the texts of the ranges: their frames follow the log open
    for ( int i = 0; i < M_range_list->count() && i < M_ranges.size(); ++i )
    {
        const QString text = rangeText( M_ranges.ranges()[i] );
        if ( M_range_list->item( i )->text() != text ) M_range_list->item( i )->setText( text );
    }

    // the frame of each end of the current range, or why not
    const ExportRangeSpec & spec = M_ranges.currentRange();
    const auto info = [&]( const bool start, QLabel * label ) {
        bool snapped = false;
        QString error;
        const ExportRange r = ExportRangeList::resolve( start
                                                        ? ExportRangeSpec{ spec.start, QString(), QString() }
                                                        : ExportRangeSpec{ QString(), spec.end, QString() },
                                                        frames, &error,
                                                        start ? &snapped : nullptr,
                                                        start ? nullptr : &snapped );
        if ( ! r.isValid() )
        {
            label->setText( frames.empty() ? QString() : error );
            label->setToolTip( label->text() );
            UIPalette::markWrongText( label, ! frames.empty() );
            return;
        }
        const rcss::rcg::ShowInfoT & show = frames[start ? r.first : r.last]->show_;
        QString text = QString( "%1 (%2)" ).arg( ExportRange::positionText( show ),
                                                 ExportRange::gameTime( show.time_, step ) );
        if ( snapped )
        {
            text += tr( ": no frame at %1" ).arg( ( start ? spec.start : spec.end ).trimmed() );
        }
        label->setText( text );
        label->setToolTip( text );
        UIPalette::markWrongText( label, false );
    };
    info( true, M_start_info );
    info( false, M_end_info );

    const bool custom = ( M_size_combo->currentData().toString() == QLatin1String( "custom" ) );
    M_width_spin->setEnabled( custom );
    M_height_spin->setEnabled( custom );
    if ( ! custom )
    {
        const QSize size = imageSize();
        const QSignalBlocker block_width( M_width_spin );
        const QSignalBlocker block_height( M_height_spin );
        M_width_spin->setValue( size.width() );
        M_height_spin->setValue( size.height() );
    }

    M_name_by_cycle_check->setEnabled( canNameByCycle() );

    // all the ranges
    QString error;
    const std::vector< ExportRange > list = ranges( &error );
    std::vector< std::vector< std::size_t > > images;
    std::size_t image_count = 0;
    std::size_t frame_count = 0;
    for ( const ExportRange & r : list )
    {
        images.push_back( imageFrames( r ) );
        image_count += images.back().size();
        frame_count += r.size();
    }
    const bool video = isVideo();
#ifdef HAVE_3D_VIEW
    if ( error.isEmpty()
         && isView3D()
         && ( imageSize().width() > OffscreenView3D::MAX_WIDTH
              || imageSize().height() > OffscreenView3D::MAX_HEIGHT ) )
    {
        error = tr( "A 3D image is %1 x %2 pixels at most." ).arg( OffscreenView3D::MAX_WIDTH ).arg( OffscreenView3D::MAX_HEIGHT );
    }
#endif
    if ( error.isEmpty() && ! list.empty() )
    {
        if ( ! video )
        {
            allImageNames( images, &error );
        }
        else
        {
            error = videoError();
        }
    }

    const bool ok = error.isEmpty() && image_count > 0;
    if ( ! ok )
    {
        M_summary_label->setText( error.isEmpty() ? tr( "There is no frame to export." ) : error );
        UIPalette::markWrongText( M_summary_label, ! frames.empty() );
    }
    else
    {
        const QSize size = imageSize();
        const QString count = ( image_count == 1
                                ? tr( "1 image" )
                                : tr( "%1 images" ).arg( image_count ) );
        const QString of = ( frame_count == 1
                             ? tr( "of 1 frame" )
                             : tr( "of %1 frames" ).arg( frame_count ) );
        const QString where = ( list.size() == 1
                                ? tr( "%1 to %2" ).arg( ExportRange::gameTime( frames[list.front().first]->show_.time_, step ),
                                                        ExportRange::gameTime( frames[list.front().last]->show_.time_, step ) )
                                : tr( "in %1 ranges" ).arg( list.size() ) );
        const QString seconds = QString::number( ExportTiming::duration( image_count, M_fps_spin->value() ), 'f', 1 );
        const int videos = videoFiles( static_cast< int >( list.size() ) ).size();
        const QString length = ( ! video
                                 ? tr( "%1 s of a video at %2 images/s" ).arg( seconds ).arg( M_fps_spin->value() )
                                 : videos == 1
                                 ? tr( "a video of %1 s at %2 images/s" ).arg( seconds ).arg( M_fps_spin->value() )
                                 : tr( "%1 videos, %2 s in all at %3 images/s" ).arg( videos ).arg( seconds ).arg( M_fps_spin->value() ) );
        M_summary_label->setText( tr( "%1 %2, %3. %4 x %5 pixels; %6." )
                                  .arg( count, of, where )
                                  .arg( size.width() ).arg( size.height() )
                                  .arg( length ) );
        UIPalette::markWrongText( M_summary_label, false );
    }

    M_export_button->setEnabled( M_allowed
                                 && ! isExporting()
                                 && ok );
    M_export_button->setToolTip( M_allowed
                                 ? QString()
                                 : tr( "Not available while connected to a server: the frames are exported after disconnecting." ) );
}

/*-------------------------------------------------------------------*/
QString
ExportDialog::videoError() const
{
    if ( M_ffmpeg_probed
         && ! M_ffmpeg_info.found )
    {
        return M_ffmpeg_info.error;
    }
    const QString ext = QFileInfo( videoFile() ).suffix().toLower();
    if ( std::none_of( std::begin( VIDEO_EXTENSIONS ), std::end( VIDEO_EXTENSIONS ),
                       [&ext]( const char * e ) { return ext == QLatin1String( e ); } ) )
    {
        return tr( "A video is a .mp4, .mov or .mkv file: not .%1." ).arg( ext );
    }
    return QString();
}

/*-------------------------------------------------------------------*/
bool
ExportDialog::prepareImages( const std::vector< std::size_t > & frames,
                             const std::vector< std::vector< std::size_t > > & by_range,
                             const bool ask,
                             QStringList * names,
                             QString * dir_out,
                             qint64 * bytes )
{
    QString error;
    *names = allImageNames( by_range, &error );
    if ( ! error.isEmpty() )
    {
        showMessage( error, true );
        return false;
    }

    const QByteArray format = M_format_combo->currentData().toString().toLatin1();

    // the files there: asked once, not for each image
    QString & dir = *dir_out;
    dir = directory();
    for ( ; ; )
    {
        int there = 0;
        for ( const QString & name : *names )
        {
            if ( QFileInfo::exists( QDir( dir ).filePath( name ) ) ) ++there;
        }
        if ( there == 0 || ! ask )
        {
            break;
        }

        QMessageBox box( QMessageBox::Question,
                         tr( "Export Images" ),
                         tr( "%1 of the %2 files are already in %3." )
                         .arg( there ).arg( names->size() ).arg( QDir::toNativeSeparators( dir ) ),
                         QMessageBox::NoButton,
                         this );
        QPushButton * overwrite = box.addButton( tr( "Overwrite" ), QMessageBox::AcceptRole );
        QPushButton * another = box.addButton( tr( "Choose Another Directory..." ), QMessageBox::ActionRole );
        box.addButton( QMessageBox::Cancel );
        box.setDefaultButton( overwrite );
        box.exec();
        if ( box.clickedButton() == overwrite )
        {
            break;
        }
        if ( box.clickedButton() != another )
        {
            return false;
        }
        const QString chosen = QFileDialog::getExistingDirectory( this,
                                                                  tr( "The directory of the images" ),
                                                                  dir );
        if ( chosen.isEmpty() )
        {
            return false;
        }
        dir = QDir::cleanPath( chosen );
        M_dir_edit->setText( dir );
    }

    // the space on the disk: the first image compressed, times the images
    qint64 & estimate = *bytes;
    estimate = 0;
    {
        QBuffer buffer;
        buffer.open( QIODevice::WriteOnly );
        QImageWriter writer( &buffer, format );
        if ( writer.write( M_canvas.renderFrame( frames.front(), imageSize(), imageRatio() ) ) )
        {
            estimate = static_cast< qint64 >( buffer.size() * 1.1 * frames.size() );
        }
    }
    const QStorageInfo storage( existingDirectory( dir ) );
    if ( ask
         && estimate > 0
         && storage.isValid()
         && storage.bytesAvailable() >= 0
         && estimate > storage.bytesAvailable() )
    {
        if ( QMessageBox::warning( this,
                                   tr( "Export Images" ),
                                   tr( "The images may not fit: about %1 for them, %2 free in %3.\nExport them anyway?" )
                                   .arg( bytesText( estimate ),
                                         bytesText( storage.bytesAvailable() ),
                                         QDir::toNativeSeparators( storage.rootPath() ) ),
                                   QMessageBox::Yes | QMessageBox::No,
                                   QMessageBox::No ) != QMessageBox::Yes )
        {
            return false;
        }
    }
    return true;
}

/*-------------------------------------------------------------------*/
bool
ExportDialog::prepareVideos( const std::vector< std::vector< std::size_t > > & by_range,
                             const std::vector< std::size_t > & frames,
                             const bool ask,
                             std::vector< ExportPart > * parts )
{
    if ( ! M_ffmpeg_probed )
    {
        probeFfmpeg();
    }
    const QString error = videoError();
    if ( ! error.isEmpty() )
    {
        showMessage( error, true );
        return false;
    }

    // the files there: asked once
    QStringList files = videoFiles( static_cast< int >( by_range.size() ) );
    for ( ; ; )
    {
        int there = 0;
        for ( const QString & file : files )
        {
            if ( QFileInfo::exists( file ) ) ++there;
        }
        if ( there == 0 || ! ask )
        {
            break;
        }

        QMessageBox box( QMessageBox::Question,
                         tr( "Export Video" ),
                         ( files.size() == 1
                           ? tr( "%1 is already there." ).arg( QDir::toNativeSeparators( files.front() ) )
                           : tr( "%1 of the %2 videos are already in %3." )
                           .arg( there ).arg( files.size() )
                           .arg( QDir::toNativeSeparators( QFileInfo( files.front() ).absolutePath() ) ) ),
                         QMessageBox::NoButton,
                         this );
        QPushButton * overwrite = box.addButton( tr( "Overwrite" ), QMessageBox::AcceptRole );
        QPushButton * another = box.addButton( tr( "Choose Another File..." ), QMessageBox::ActionRole );
        box.addButton( QMessageBox::Cancel );
        box.setDefaultButton( overwrite );
        box.exec();
        if ( box.clickedButton() == overwrite )
        {
            break;
        }
        if ( box.clickedButton() != another )
        {
            return false;
        }
        const QString chosen = QFileDialog::getSaveFileName( this,
                                                             tr( "The video file" ),
                                                             videoFile(),
                                                             tr( "Videos (*.mp4 *.mov *.mkv)" ),
                                                             nullptr,
                                                             QFileDialog::DontConfirmOverwrite );
        if ( chosen.isEmpty() )
        {
            return false;
        }
        M_video_edit->setText( chosen );
        const QString e = videoError();
        if ( ! e.isEmpty() )
        {
            showMessage( e, true );
            return false;
        }
        files = videoFiles( static_cast< int >( by_range.size() ) );
    }

    // a video of the 2D view: flat colors (-tune animation); of the 3D view: as a film
    const bool animation = ! isView3D();
    const auto sink = [this, animation]( const QString & file ) {
        return new FfmpegSink( M_ffmpeg_info.program, file, M_ffmpeg_info.encoder,
                               M_crf_spin->value(), M_preset_combo->currentText(), animation );
    };
    if ( files.size() == 1 )
    {
        parts->push_back( ExportPart{ sink( files.front() ), frames } );
    }
    else
    {
        for ( int r = 0; r < files.size(); ++r )
        {
            parts->push_back( ExportPart{ sink( files[r] ), by_range[static_cast< std::size_t >( r )] } );
        }
    }
    return true;
}

/*-------------------------------------------------------------------*/
bool
ExportDialog::startExport( const bool ask )
{
    if ( isExporting() )
    {
        return false;
    }
    if ( ! M_allowed )
    {
        showMessage( tr( "Not available while connected to a server." ), true );
        return false;
    }

    QString error;
    const std::vector< ExportRange > list = ranges( &error );
    std::vector< std::vector< std::size_t > > by_range;
    std::vector< std::size_t > frames;
    for ( const ExportRange & r : list )
    {
        by_range.push_back( imageFrames( r ) );
        frames.insert( frames.end(), by_range.back().begin(), by_range.back().end() );
    }
    if ( ! error.isEmpty() || frames.empty() )
    {
        showMessage( error.isEmpty() ? tr( "There is no frame to export." ) : error, true );
        return false;
    }

    const QSize pixels = imageSize();
    const qreal dpr = imageRatio();
    std::vector< ExportPart > parts;
    qint64 estimate = 0;

    // the 3D view drawn offscreen: its context first (nothing written without it)
    const bool view_3d = isView3D();
#ifdef HAVE_3D_VIEW
    if ( view_3d )
    {
        QString gl_error;
        if ( ! M_offscreen_3d->initialize( &gl_error ) )
        {
            showMessage( gl_error, true );
            return false;
        }
    }
#endif

    if ( isVideo() )
    {
        if ( ! prepareVideos( by_range, frames, ask, &parts ) )
        {
            return false;
        }
    }
    else
    {
        QStringList names;
        QString dir;
        if ( ! prepareImages( frames, by_range, ask, &names, &dir, &estimate ) )
        {
            return false;
        }
        parts.push_back( ExportPart{ new ImageSequenceSink( dir, names, M_format_combo->currentData().toString().toLatin1() ),
                                     frames } );
    }

    // the first image of each range: the 3D camera jumps there. 3D: the
    // positions of the images between the frames.
    {
        std::vector< std::vector< double > > positions;
        for ( const ExportRange & r : list )
        {
            positions.push_back( view_3d ? imagePositions( r ) : std::vector< double >() );
        }
        if ( parts.size() == 1 )
        {
            std::size_t start = 0;
            for ( std::size_t r = 0; r < list.size(); ++r )
            {
                parts.front().range_starts.push_back( start );
                parts.front().positions.insert( parts.front().positions.end(), positions[r].begin(), positions[r].end() );
                start += by_range[r].size();
            }
        }
        else
        {
            for ( std::size_t r = 0; r < parts.size() && r < positions.size(); ++r )
            {
                parts[r].positions = positions[r];
            }
        }
    }

    writeSettings();
    emit stopRequested();

    M_job_video = isVideo();
    ExportJob * job = nullptr;
#ifdef HAVE_3D_VIEW
    if ( view_3d )
    {
        // the camera as the view's (or the camera of the options), jumping at the first image
        CameraController camera;
        if ( M_view_3d )
        {
            camera = M_view_3d->camera();
        }
        else
        {
            camera.setMode( Options::instance().camera3D() );
        }
        M_offscreen_3d->start( camera, M_fps_spin->value() );
        job = new ExportJob( *M_offscreen_3d, parts, pixels, dpr, M_fps_spin->value(), this );
    }
#endif
    if ( ! job )
    {
        job = new ExportJob( M_canvas, parts, pixels, dpr, M_fps_spin->value(), this );
    }
    M_job = job;

    const QString title = ( M_job_video ? tr( "Export Video" ) : tr( "Export Images" ) );
    const QString text = ( ! M_job_video
                           ? tr( "Exporting %1 images (about %2)..." ).arg( frames.size() ).arg( bytesText( estimate ) )
                           : parts.size() == 1
                           ? tr( "Exporting %1 images to the video..." ).arg( frames.size() )
                           : tr( "Exporting %1 images to %2 videos..." ).arg( frames.size() ).arg( parts.size() ) );
    QProgressDialog * progress = new QProgressDialog( text,
                                                      tr( "Stop" ),
                                                      0, static_cast< int >( frames.size() ),
                                                      this );
    progress->setObjectName( "export_progress" );
    progress->setWindowTitle( title );
    progress->setWindowModality( Qt::ApplicationModal );
    progress->setMinimumDuration( 500 );
    progress->setAutoClose( false );
    progress->setAutoReset( false );
    M_progress = progress;
    connect( progress, &QProgressDialog::canceled,
             this, &ExportDialog::abortExport );
    std::shared_ptr< QElapsedTimer > clock = std::make_shared< QElapsedTimer >();
    clock->start();
    connect( job, &ExportJob::progressed,
             this, [this, clock]( std::size_t done, std::size_t total ) {
                 if ( ! M_progress ) return;
                 QString text = tr( "Exported %1 of %2 images." ).arg( done ).arg( total );
                 if ( done >= total )
                 {
                     text = ( M_job_video
                              ? tr( "Finishing the video..." )
                              : tr( "Exported %1 images." ).arg( total ) );
                 }
                 else if ( done > 0
                           && clock->elapsed() >= 2000 )
                 {
                     // the time left at the speed so far
                     text += QLatin1Char( ' ' )
                         + leftText( static_cast< qint64 >( clock->elapsed() * static_cast< double >( total - done ) / done ) );
                 }
                 // the label first: setValue() of a modal dialog processes the events
                 M_progress->setLabelText( text );
                 M_progress->setValue( static_cast< int >( done ) );
             } );
    connect( job, &ExportJob::finished,
             this, &ExportDialog::handleFinished );

    showMessage( tr( "Exporting..." ), false );
    updateSummary();
    job->start();
    return true;
}
