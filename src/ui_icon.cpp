// -*-c++-*-

/*!
  \file ui_icon.cpp
  \brief the icons that follow the color scheme. Source File.
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

#include "ui_icon.h"

#include "ui_palette.h"

#include <QApplication>
#include <QIconEngine>
#include <QLibraryInfo>
#include <QList>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QStyle>
#include <QStyleOption>
#include <QVersionNumber>

const int UIIcon::SIZES[4] = { 16, 24, 32, 48 };
const int UIIcon::APP_SIZES[6] = { 16, 32, 48, 64, 128, 256 };

namespace {

/*-------------------------------------------------------------------*/
/*!
  \brief register the embedded icons once
*/
void
ensure_resources()
{
    static const bool s_initialized = ( UIIcon::initResources(), true );
    Q_UNUSED( s_initialized );
}

/*-------------------------------------------------------------------*/
/*!
  \brief the pixmap of a mode, made by the style: the disabled and the
  selected ones
  \param pix the normal pixmap
  \param mode the mode of the icon
  \param palette the palette of the application
  \return the pixmap of the mode
*/
QPixmap
mode_pixmap( const QPixmap & pix,
             const QIcon::Mode mode,
             const QPalette & palette )
{
    if ( mode == QIcon::Disabled
         || mode == QIcon::Selected )
    {
        if ( QStyle * style = QApplication::style() )
        {
            QStyleOption opt( 0 );
            opt.palette = palette;
            return style->generatedIconPixmap( mode, pix, &opt );
        }
    }
    return pix;
}

/*-------------------------------------------------------------------*/
/*!
  \class SchemeIconEngine
  \brief the icon engine that chooses the images of the light or the dark
  color scheme whenever the icon is painted (UIIcon::fromName())
*/
class SchemeIconEngine
    : public QIconEngine {
private:
    QList< QPixmap > M_light; //!< the images of the light color scheme (in ascending order of the size)
    QList< QPixmap > M_dark; //!< the images of the dark color scheme (in ascending order of the size)

public:

    SchemeIconEngine( const QList< QPixmap > & light,
                      const QList< QPixmap > & dark )
        : QIconEngine(),
          M_light( light ),
          M_dark( dark )
      { }

    SchemeIconEngine( const SchemeIconEngine & other )
        : QIconEngine( other ),
          M_light( other.M_light ),
          M_dark( other.M_dark )
      { }

private:

    const QList< QPixmap > & images( const QPalette & palette ) const
      {
          return UIPalette::isDark( palette ) ? M_dark : M_light;
      }

    /*!
      \brief the size in which the icon is drawn: the largest of the images
      that fits in the size asked. Never larger than the largest image (as
      QIcon( QPixmap )), and smaller than the smallest only if the size
      asked is.
      \param images the images in ascending order of the size
      \param size the size asked, in device independent pixels
      \return the size in device independent pixels
    */
    static
    QSize drawnSize( const QList< QPixmap > & images,
                     const QSize & size )
      {
          QSize result;
          for ( const QPixmap & pix : images )
          {
              if ( pix.width() <= size.width()
                   && pix.height() <= size.height() )
              {
                  result = pix.size();
              }
          }

          if ( result.isEmpty()
               && ! images.isEmpty() )
          {
              // smaller than the smallest image
              result = images.front().size();
              result.scale( size, Qt::KeepAspectRatio );
          }
          return result;
      }

    /*!
      \brief the image of a size: the image of that size, or else the
      nearest larger one (or the largest), scaled
      \param images the images in ascending order of the size
      \param size the size in device pixels
      \return the image
    */
    static
    QPixmap selectImage( const QList< QPixmap > & images,
                         const QSize & size )
      {
          const QPixmap * best = &images.back();
          for ( const QPixmap & pix : images )
          {
              if ( pix.width() >= size.width()
                   && pix.height() >= size.height() )
              {
                  best = &pix;
                  break;
              }
          }

          if ( best->size() == size )
          {
              return *best;
          }
          return best->scaled( size, Qt::KeepAspectRatio, Qt::SmoothTransformation );
      }

    /*!
      \brief the pixmap of the palette of the application now
      \param size the size asked, in device independent pixels
      \param scale the device pixel ratio
      \param mode the mode of the icon
      \param drawn_size if not null, the size in which the icon is drawn (in
      device independent pixels)
      \return the pixmap in device pixels (no device pixel ratio set)
    */
    QPixmap createPixmap( const QSize & size,
                          const qreal scale,
                          const QIcon::Mode mode,
                          QSize * drawn_size = nullptr ) const
      {
          const QPalette palette = QGuiApplication::palette();
          const QList< QPixmap > & imgs = images( palette );

          const QSize drawn = drawnSize( imgs, size );
          if ( drawn_size )
          {
              *drawn_size = drawn;
          }
          if ( drawn.isEmpty() )
          {
              return QPixmap();
          }

          const QSize device_size( qRound( drawn.width() * scale ),
                                   qRound( drawn.height() * scale ) );
          return mode_pixmap( selectImage( imgs, device_size ), mode, palette );
      }

public:

    QSize actualSize( const QSize & size,
                      QIcon::Mode,
                      QIcon::State ) override
      {
          return drawnSize( images( QGuiApplication::palette() ), size );
      }

    QPixmap pixmap( const QSize & size,
                    QIcon::Mode mode,
                    QIcon::State ) override
      {
          return createPixmap( size, 1.0, mode );
      }

#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
    QPixmap scaledPixmap( const QSize & size,
                          QIcon::Mode mode,
                          QIcon::State,
                          qreal scale ) override
      {
          // Since Qt 6.8, the size is in device independent pixels; before,
          // it is multiplied by the scale (device pixels). The version of the
          // library at run time is checked: the program may run with a newer
          // Qt than the one it was built with.
          static const bool logical_size = ( QLibraryInfo::version() >= QVersionNumber( 6, 8, 0 ) );
          if ( logical_size
               || scale <= 0.0 )
          {
              return createPixmap( size, scale > 0.0 ? scale : 1.0, mode );
          }
          return createPixmap( size / scale, scale, mode );
      }

    QList< QSize > availableSizes( QIcon::Mode,
                                   QIcon::State ) override
#else
    QList< QSize > availableSizes( QIcon::Mode,
                                   QIcon::State ) const override
#endif
      {
          QList< QSize > sizes;
          for ( const QPixmap & pix : images( QGuiApplication::palette() ) )
          {
              sizes << pix.size();
          }
          return sizes;
      }

    void paint( QPainter * painter,
                const QRect & rect,
                QIcon::Mode mode,
                QIcon::State ) override
      {
          const qreal scale = ( painter->device()
                                ? painter->device()->devicePixelRatioF()
                                : 1.0 );
          QSize drawn;
          QPixmap pix = createPixmap( rect.size(), scale, mode, &drawn );
          if ( pix.isNull() )
          {
              return;
          }

          pix.setDevicePixelRatio( scale );
          painter->drawPixmap( QStyle::alignedRect( Qt::LeftToRight, Qt::AlignCenter, drawn, rect ),
                               pix );
      }

    QIconEngine * clone() const override
      {
          return new SchemeIconEngine( *this );
      }

    QString key() const override
      {
          return QStringLiteral( "SchemeIconEngine" );
      }

#if QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
    void virtual_hook( int id,
                       void * data ) override
      {
          if ( id == QIconEngine::ScaledPixmapHook )
          {
              // Qt5 gives the size in device pixels.
              QIconEngine::ScaledPixmapArgument & arg = *reinterpret_cast< QIconEngine::ScaledPixmapArgument * >( data );
              const qreal scale = ( arg.scale > 0.0 ? arg.scale : 1.0 );
              arg.pixmap = createPixmap( arg.size / scale, scale, arg.mode );
              return;
          }
          QIconEngine::virtual_hook( id, data );
      }
#endif
};

}

/*-------------------------------------------------------------------*/
/*!

*/
void
UIIcon::initResources()
{
    // icons/icons.qrc: linked into the static library rcssmonitor_core, so
    // registered explicitly (as the 3D view's resources)
    Q_INIT_RESOURCE( icons );
}

/*-------------------------------------------------------------------*/
/*!

*/
QIcon
UIIcon::fromName( const QString & name )
{
    ensure_resources();

    QList< QPixmap > light;
    QList< QPixmap > dark;
    for ( const int size : SIZES )
    {
        const QString file = QStringLiteral( "/%1/%2.png" ).arg( size ).arg( name );
        light << QPixmap( QStringLiteral( ":/icons/light" ) + file );
        dark << QPixmap( QStringLiteral( ":/icons/dark" ) + file );
        if ( light.back().isNull()
             || dark.back().isNull() )
        {
            qWarning( "UIIcon: the icon \"%s\" is not found in the resources (%s).",
                      qPrintable( name ), qPrintable( QStringLiteral( ":/icons/{light,dark}" ) + file ) );
            return QIcon();
        }
    }

    return QIcon( new SchemeIconEngine( light, dark ) );
}

/*-------------------------------------------------------------------*/
/*!

*/
QIcon
UIIcon::appIcon( const QString & name )
{
    ensure_resources();

    QIcon icon;
    for ( const int size : APP_SIZES )
    {
        const QString file = QStringLiteral( ":/icons/app/%1/%2.png" ).arg( size ).arg( name );
        const QPixmap pix( file );
        if ( pix.isNull() )
        {
            qWarning( "UIIcon: the application icon \"%s\" is not found in the resources (%s).",
                      qPrintable( name ), qPrintable( file ) );
            return QIcon();
        }
        // addPixmap() does not look for the "@2x" files, unlike addFile()
        icon.addPixmap( pix );
    }
    return icon;
}
