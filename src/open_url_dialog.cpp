// -*-c++-*-

/*!
  \file open_url_dialog.cpp
  \brief the dialog to open a game log from a URL. Source File.
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

#include "open_url_dialog.h"

#include "log_url.h"
#include "ui_palette.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

/*-------------------------------------------------------------------*/
OpenUrlDialog::OpenUrlDialog( QWidget * parent,
                              const QStringList & recent,
                              const QString & cache_dir )
    : QDialog( parent ),
      M_cache_dir( cache_dir )
{
    this->setWindowTitle( tr( "Open Game Log from URL" ) );

    QVBoxLayout * layout = new QVBoxLayout();
    // as tall as the texts (the error shown, a long path)
    layout->setSizeConstraint( QLayout::SetMinimumSize );

    QLabel * label = new QLabel( tr( "The URL of a game log (.rcg or .rcg.gz), http:// or https://. "
                                     "A file page on GitHub is read from its raw file." ) );
    label->setWordWrap( true );
    layout->addWidget( label );

    M_url_box = new QComboBox();
    M_url_box->setObjectName( "url" );
    M_url_box->setEditable( true );
    M_url_box->setInsertPolicy( QComboBox::NoInsert );
    M_url_box->addItems( recent );
    M_url_box->setCurrentIndex( -1 );
    M_url_box->setEditText( QString() );
    M_url_box->lineEdit()->setPlaceholderText( QStringLiteral( "https://github.com/OWNER/REPO/blob/main/logs/game.rcg.gz" ) );
    M_url_box->setMinimumWidth( M_url_box->fontMetrics().horizontalAdvance( QLatin1Char( 'x' ) ) * 64 );
    label->setBuddy( M_url_box );
    layout->addWidget( M_url_box );

    M_error_label = new QLabel();
    M_error_label->setObjectName( "error" );
    M_error_label->setWordWrap( true );
    M_error_label->hide();
    layout->addWidget( M_error_label );

    // the cache of the downloaded logs: reused for the same URL
    {
        QHBoxLayout * row = new QHBoxLayout();
        M_cache_label = new QLabel();
        M_cache_label->setObjectName( "cache" );
        M_cache_label->setWordWrap( true );
        M_cache_label->setTextInteractionFlags( Qt::TextSelectableByMouse );
        row->addWidget( M_cache_label, 1 );
        M_clear_cache_button = new QPushButton( tr( "Clear Cache" ) );
        M_clear_cache_button->setObjectName( "clear_cache" );
        M_clear_cache_button->setAutoDefault( false );
        M_clear_cache_button->setToolTip( tr( "Remove the downloaded logs. A URL opened again is downloaded again." ) );
        connect( M_clear_cache_button, &QPushButton::clicked,
                 this, &OpenUrlDialog::clearCache );
        row->addWidget( M_clear_cache_button );
        layout->addLayout( row );
    }

    QDialogButtonBox * buttons = new QDialogButtonBox( QDialogButtonBox::Open | QDialogButtonBox::Cancel );
    connect( buttons, &QDialogButtonBox::accepted,
             this, &OpenUrlDialog::accept );
    connect( buttons, &QDialogButtonBox::rejected,
             this, &OpenUrlDialog::reject );
    layout->addWidget( buttons );

    // the error goes away when the URL is typed again
    connect( M_url_box, &QComboBox::editTextChanged,
             this, [this]( const QString & ) {
                 UIPalette::markWrongText( M_error_label, false );
                 M_error_label->hide();
             } );

    this->setLayout( layout );
    updateCacheLabel();
}

/*-------------------------------------------------------------------*/
QString
OpenUrlDialog::text() const
{
    return M_url_box->currentText();
}

/*-------------------------------------------------------------------*/
void
OpenUrlDialog::setText( const QString & text )
{
    M_url_box->setEditText( text );
}

/*-------------------------------------------------------------------*/
QString
OpenUrlDialog::formatBytes( const qint64 bytes )
{
    if ( bytes < 1024 )
    {
        return tr( "%1 bytes" ).arg( bytes );
    }
    if ( bytes < 1024 * 1024 )
    {
        return tr( "%1 KB" ).arg( bytes / 1024.0, 0, 'f', 1 );
    }
    return tr( "%1 MB" ).arg( bytes / ( 1024.0 * 1024.0 ), 0, 'f', 1 );
}

/*-------------------------------------------------------------------*/
void
OpenUrlDialog::accept()
{
    QString error;
    const QUrl url = parseLogUrl( text(), &error );
    if ( ! url.isValid() )
    {
        M_error_label->setText( error );
        UIPalette::markWrongText( M_error_label, true );
        M_error_label->show();
        M_url_box->setFocus();
        return;
    }

    M_url = url;
    QDialog::accept();
}

/*-------------------------------------------------------------------*/
void
OpenUrlDialog::clearCache()
{
    if ( ! clearLogCache( M_cache_dir ) )
    {
        QMessageBox::warning( this,
                              tr( "Clear Cache" ),
                              tr( "Cannot remove %1." ).arg( QDir::toNativeSeparators( M_cache_dir ) ) );
    }
    updateCacheLabel();
}

/*-------------------------------------------------------------------*/
void
OpenUrlDialog::updateCacheLabel()
{
    const qint64 size = logCacheSize( M_cache_dir );
    // the home as ~, shorter
    QString dir = QDir::toNativeSeparators( M_cache_dir );
    const QString home = QDir::toNativeSeparators( QDir::homePath() );
    if ( dir.startsWith( home + QDir::separator() ) )
    {
        dir = QLatin1Char( '~' ) + dir.mid( home.size() );
    }
    M_cache_label->setText( tr( "Downloaded logs are kept in %1 (%2)." ).arg( dir, formatBytes( size ) ) );
    M_cache_label->setToolTip( QDir::toNativeSeparators( M_cache_dir ) );
    M_clear_cache_button->setEnabled( size > 0 );
}
