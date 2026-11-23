// -*-c++-*-

/*!
  \file open_url_dialog.h
  \brief the dialog to open a game log from a URL. Header File.
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

#ifndef RCSSMONITOR_OPEN_URL_DIALOG_H
#define RCSSMONITOR_OPEN_URL_DIALOG_H

#include <QDialog>
#include <QString>
#include <QStringList>
#include <QUrl>

class QComboBox;
class QLabel;
class QPushButton;

/*!
  \class OpenUrlDialog
  \brief File > Open Game Log from URL (docs/plan/2026-11-22-02_remote-logs.md
  4.1): the URL of a game log, with the ones opened recently, and the cache
  of the downloaded logs (its size, and a button to clear it). Open is
  accepted only for a URL that parseLogUrl() takes; otherwise the reason is
  shown in the dialog.
*/
class OpenUrlDialog
    : public QDialog {

    Q_OBJECT

private:

    QString M_cache_dir;
    QComboBox * M_url_box;
    QLabel * M_error_label;
    QLabel * M_cache_label;
    QPushButton * M_clear_cache_button;
    QUrl M_url;

public:

    /*!
      \param parent the parent widget
      \param recent the URLs opened recently, the newest first
      \param cache_dir the cache directory of the downloaded logs
    */
    OpenUrlDialog( QWidget * parent,
                   const QStringList & recent,
                   const QString & cache_dir );

    //! the text typed or chosen
    QString text() const;
    void setText( const QString & text );

    //! the URL to download, when accepted (parseLogUrl())
    const QUrl & url() const { return M_url; }

    /*!
      \brief the size of a cache, to show: "12.3 MB"
    */
    static QString formatBytes( const qint64 bytes );

public slots:

    void accept() override;

private slots:

    void clearCache();
    void updateCacheLabel();
};

#endif
