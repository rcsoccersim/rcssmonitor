// -*-c++-*-

/*!
  \file field_view_3d.cpp
  \brief the 3D view: a QOpenGLWidget. Source File.
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

#include "field_view_3d.h"

#include "ground_texture.h"
#include "guides_3d.h"
#include "interpolate.h"
#include "overlay_builder.h"
#include "picking.h"
#include "stadium_3d.h"
#include "scene_builder.h"

#include "disp_holder.h"
#include "field_painter.h"
#include "log_player.h"
#include "options.h"
#include "player_selection.h"
#include "score_board_painter.h"
#include "team_graphic_painter.h"

#include <QContextMenuEvent>
#include <QFontInfo>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions_3_3_Core>
#include <QPainter>

#include <cmath>
#include <iostream>

/*-------------------------------------------------------------------*/
FieldView3D::FieldView3D( DispHolder & disp_holder,
                          const LogPlayer & log_player,
                          QWidget * parent )
    : QOpenGLWidget( parent ),
      M_disp_holder( disp_holder ),
      M_log_player( log_player ),
      M_field_painter( new FieldPainter( disp_holder ) ),
      M_score_board_painter( new ScoreBoardPainter( disp_holder ) ),
      M_team_graphic_painter( new TeamGraphicPainter( disp_holder ) ),
      M_ready( false ),
      M_newest( nullptr ),
      M_moving( false )
{
    setFormat( surfaceFormat() );
    setFocusPolicy( Qt::WheelFocus );

    // While things move, a frame is drawn as soon as the one before is
    // shown: paced by the display (60 times a second on most screens),
    // without a timer of our own.
    connect( this, SIGNAL( frameSwapped() ),
             this, SLOT( drawNext() ) );
}

/*-------------------------------------------------------------------*/
FieldView3D::~FieldView3D()
{
    // the GPU's resources are deleted in their context
    makeCurrent();
    M_scene.release();
    doneCurrent();
}

/*-------------------------------------------------------------------*/
QSurfaceFormat
FieldView3D::surfaceFormat()
{
    QSurfaceFormat format;
    format.setVersion( 3, 3 );
    format.setProfile( QSurfaceFormat::CoreProfile );
    format.setDepthBufferSize( 24 );
    format.setStencilBufferSize( 8 );
    format.setSamples( 4 );
    return format;
}

/*-------------------------------------------------------------------*/
QString
FieldView3D::unavailableReason()
{
    // Try what the widget will need, before it is made: a context of OpenGL
    // 3.3 core with its functions. Without a GPU or with an old driver, it
    // fails here, and the 2D view stays (MainWindow::toggleView3D()).

    // these platforms show no OpenGL widgets
    const QString platform = QGuiApplication::platformName();
    if ( platform == "offscreen"
         || platform == "minimal"
         || platform == "vnc"
         || platform == "linuxfb" )
    {
        return QObject::tr( "The platform \"%1\" does not show OpenGL widgets." ).arg( platform );
    }

    QOpenGLContext context;
    context.setFormat( surfaceFormat() );
    if ( ! context.create() )
    {
        return QObject::tr( "OpenGL is not available." );
    }
    const QPair< int, int > version = context.format().version();
    if ( version < qMakePair( 3, 3 ) )
    {
        return QObject::tr( "OpenGL 3.3 is not available (%1.%2)." ).arg( version.first ).arg( version.second );
    }

    QOffscreenSurface surface;
    surface.setFormat( context.format() );
    surface.create();
    if ( ! context.makeCurrent( &surface ) )
    {
        return QObject::tr( "OpenGL is not available." );
    }
    QOpenGLFunctions_3_3_Core functions;
    const bool ok = functions.initializeOpenGLFunctions();
    context.doneCurrent();
    return ok ? QString() : QObject::tr( "OpenGL 3.3 core is not available." );
}

/*-------------------------------------------------------------------*/
void
FieldView3D::initializeGL()
{
    M_ready = M_scene.initialize();
    if ( ! M_ready )
    {
        std::cerr << "(FieldView3D) OpenGL 3.3 core is not available" << std::endl;
    }
    M_ground_key.clear();
    M_stadium_keys[0].clear();
    M_stadium_keys[1].clear();
    // the new context has no texture of the labels yet
    M_label_atlas = LabelAtlas();
}

/*-------------------------------------------------------------------*/
void
FieldView3D::paintGL()
{
    if ( ! M_ready )
    {
        return;
    }

    // the ground image: drawn again only when its colors or options changed
    const QString key = GroundTexture::key( M_disp_holder.serverParam() );
    if ( key != M_ground_key )
    {
        M_scene.setGround( GroundTexture::draw( *M_field_painter ) );
        M_ground_key = key;
    }

    // The frames to draw, and how far between them (§5.2). Live, the
    // fraction is the time since the newest frame came, in simulation steps.
    const DispCont & frames = M_disp_holder.dispCont();
    const rcss::rcg::DispInfoT * newest = ( frames.empty() ? nullptr : frames.back().get() );
    if ( newest != M_newest )
    {
        M_newest = newest;
        M_newest_clock.start();
    }
    const bool live = M_log_player.isLiveMode();
    const double step = std::max( 1, M_disp_holder.serverParam().simulator_step_ );
    const double since_newest = ( M_newest_clock.isValid() ? M_newest_clock.nsecsElapsed() * 1.0e-6 / step : 1.0 );
    const Interpolate::Position at = Interpolate::position( frames.size(),
                                                            M_disp_holder.currentIndex(),
                                                            live,
                                                            M_log_player.alpha(),
                                                            since_newest );

    // what to draw where: the goals, and the players and the ball on the way
    // between the two frames
    // the automatic selection of a player, as the 2D view does when it is drawn
    if ( const DispConstPtr current = M_disp_holder.currentDisp() )
    {
        selectPlayerNearestBall( current->show_, Options::instance() );
    }

    // The ends in the team colors: the nets, and the boards and the banners
    // with the team names and logos, drawn again when they change
    const Options & opt = Options::instance();
    const QColor team_colors[2] = { opt.leftTeamBrush().color(), opt.rightTeamBrush().color() };
    {
        const DispConstPtr current = M_disp_holder.currentDisp();
        for ( int end = 0; end < 2; ++end )
        {
            Stadium3D::TeamLook look;
            look.color = team_colors[end];
            look.name = ( current ? QString::fromStdString( current->team_[end].name_ ) : QString() );
            if ( opt.showTeamGraphic() )
            {
                look.logo = M_team_graphic_painter->pixmap( end == 0 ).toImage();
            }
            const QString key = look.key();
            if ( key != M_stadium_keys[end] )
            {
                M_scene.setStadium( end, Stadium3D::boardImage( look ), Stadium3D::bannerImage( look ) );
                M_stadium_keys[end] = key;
            }
        }
    }

    M_content.clear();
    SceneBuilder::addGoals( M_content, team_colors[0], team_colors[1] );
    Stadium3D::addPanels( M_content, opt.boards3D(), opt.banners3D(), team_colors[0], team_colors[1] );
    rcss::rcg::ShowInfoT show;
    if ( ! frames.empty() )
    {
        show = Interpolate::show( *frames[at.index],
                                  ( at.next != at.index ? frames[at.next].get() : nullptr ),
                                  at.alpha );
    }

    // what the cameras follow: the ball, and the selected player in the show
    CameraSubject subject;
    subject.ball_x = show.ball_.x_;
    subject.ball_y = show.ball_.y_;
    int selected = -1;
    if ( ! frames.empty() )
    {
        for ( int i = 0; i < rcss::rcg::MAX_PLAYER * 2; ++i )
        {
            const rcss::rcg::PlayerT & p = show.player_[i];
            if ( p.side() != rcss::rcg::NEUTRAL
                 && p.isAlive()
                 && Options::instance().selectedPlayer( p.side(), p.unum_ ) )
            {
                selected = i;
                subject.has_player = true;
                subject.player_x = p.x_;
                subject.player_y = p.y_;
                subject.player = &show.player_[i];
            }
        }
    }

    // The player camera draws what its player perceives (its sight),
    // decided on the frame of the cycle, not between frames; at its eyes,
    // not the player itself.
    const PlayerCameraOptions & player_camera = Options::instance().playerCamera();
    Perceptions perceptions;
    const Perceptions * seen = nullptr;
    if ( M_camera.mode() == CameraMode::Player
         && selected >= 0 )
    {
        const rcss::rcg::ShowInfoT & cycle = frames[at.index]->show_;
        perceptions = Perceptions::perceive( player_camera.sight,
                                             cycle,
                                             selected,
                                             M_disp_holder.playerType( cycle.player_[selected].type_ ),
                                             M_disp_holder.serverParam(),
                                             player_camera.position == PlayerCameraStyle::Position::Eye );
        seen = &perceptions;
    }

    if ( ! frames.empty() )
    {
        SceneBuilder::addShow( M_content, show, M_disp_holder, Options::instance(), seen );
        OverlayBuilder::addDirections( M_content, show, M_disp_holder, Options::instance(), seen );
        OverlayBuilder::addViewAreas( M_content, show, M_disp_holder, Options::instance(), seen );
        OverlayBuilder::addOverlays( M_content, show, M_disp_holder, Options::instance(), seen );
    }

    // the camera, after the ball for the time since the last frame
    const double dt = ( M_frame_clock.isValid() ? M_frame_clock.restart() * 1.0e-3 : 0.0 );
    if ( ! M_frame_clock.isValid() ) M_frame_clock.start();
    const float aspect = float( width() ) / float( std::max( 1, height() ) );
    M_camera.setCloseup( float( Options::instance().closeupDistance3D() ),
                         float( Options::instance().closeupElevation3D() ) );
    M_camera.setPlayerCamera( player_camera );
    const CameraPose camera = M_camera.update( dt, aspect, subject );
    M_drawn_camera = camera;
    M_drawn = true;

    // draw again after this frame (drawNext()) while playing a log, live
    // until the newest frame is reached, and while the camera glides
    M_moving = ( M_log_player.isPlaying()
                 || ( live && since_newest < 1.0 )
                 || ! M_camera.settled() );

    // the number labels and the cards, sized for the camera and the screen
    if ( ! frames.empty() )
    {
        const SceneBuilder::LabelView label_view = {
            camera,
            float( height() ),
            double( QFontInfo( Options::instance().playerFont() ).pixelSize() ),
        };
        SceneBuilder::addLabels( M_content, show, M_disp_holder, Options::instance(), M_label_atlas, label_view, seen );
    }
    if ( M_label_atlas.takeChanged() )
    {
        M_scene.setLabels( M_label_atlas.image() );
    }

    // draw into the widget's framebuffer, in physical pixels: on a screen
    // scaled by 1.5, a widget of 100 logical pixels has 150 of them
    const qreal dpr = devicePixelRatioF();
    M_scene.render( defaultFramebufferObject(),
                    static_cast< int >( std::lround( width() * dpr ) ),
                    static_cast< int >( std::lround( height() * dpr ) ),
                    camera,
                    M_content );

    // the HUD over the scene, as the 2D view draws it. The team logo on the
    // right is placed by the size of the canvas in the options, which the
    // 2D view sets when it is drawn: set it here, the same size.
    if ( ! frames.empty() )
    {
        Options::instance().updateFieldSize( width(), height() );
        QPainter painter( this );
        // on a scaled screen, a painter of QOpenGLWidget has its window in
        // physical pixels while it draws in logical ones: the score board,
        // placed at the bottom of the window, fell off the view. The window
        // in logical pixels, as a painter of the 2D view has it.
        painter.setViewport( rect() );
        painter.setWindow( rect() );
        M_score_board_painter->draw( painter );
        M_team_graphic_painter->draw( painter );

        // the texts of the overlays, beside their points, in the player font
        if ( ! M_content.texts.empty() )
        {
            const QFont & font = Options::instance().playerFont();
            painter.setFont( font );
            const QMatrix4x4 view_projection = camera.projection( aspect ) * camera.view();
            for ( const OverlayBuilder::PlacedText & t : OverlayBuilder::placeTexts( M_content.texts, view_projection, size(), QFontMetrics( font ) ) )
            {
                painter.setPen( t.color );
                painter.drawText( t.baseline, t.text );
            }
        }

        // the guides: the compass at the top, the minimap at the bottom right
        const bool compass = Guides3D::shown( opt.compass3D(), M_camera.mode() );
        const bool minimap = Guides3D::shown( opt.minimap3D(), M_camera.mode() );
        if ( compass || minimap )
        {
            const Guides3D::CameraGuide guide = Guides3D::guideOf( camera, aspect );
            if ( compass )
            {
                const QPointF ball( show.ball_.x_, show.ball_.y_ );
                // the ball as the camera's player perceives it
                const bool ball_seen = ( ! seen || seen->ball );
                Guides3D::drawCompass( painter, Guides3D::compassRect( width() ), guide,
                                       ball_seen ? &ball : nullptr, opt );
            }
            if ( minimap )
            {
                double scale = 1.0;
                const QRectF rect = Guides3D::minimapRect( width(), height(), &scale );
                // the pitch, drawn again only when the ground or the size changes, in device pixels
                const QString key = M_ground_key + '|' + QString::number( rect.width() ) + '|' + QString::number( dpr );
                if ( key != M_minimap_key )
                {
                    M_minimap_field = GroundTexture::draw( *M_field_painter, scale * dpr, 1.0 );
                    M_minimap_key = key;
                }
                Guides3D::drawMinimap( painter, rect, scale, guide, &show, M_minimap_field, opt );
            }
        }
    }
}

/*-------------------------------------------------------------------*/
void
FieldView3D::setCameraMode( const CameraMode mode )
{
    M_camera.setMode( mode );
    update();
}

/*-------------------------------------------------------------------*/
void
FieldView3D::zoomCamera( const double factor )
{
    if ( factor <= 0.0 )
    {
        M_camera.resetZoom();
    }
    else
    {
        M_camera.zoom( float( factor ) );
    }
    // the close-up cameras' distance is a setting, which stays
    if ( isCloseup( M_camera.mode() ) )
    {
        Options::instance().setCloseupDistance3D( M_camera.closeupDistance() );
    }
    update();
}

/*-------------------------------------------------------------------*/
void
FieldView3D::mousePressEvent( QMouseEvent * event )
{
    // where a drag starts (the right button is left for the menu, Phase 5)
    M_last_mouse = event->pos();
    QOpenGLWidget::mousePressEvent( event );
}

/*-------------------------------------------------------------------*/
void
FieldView3D::mouseMoveEvent( QMouseEvent * event )
{
    // the drag since the last move, in logical pixels as the height
    const QPoint delta = event->pos() - M_last_mouse;
    M_last_mouse = event->pos();
    const float dx = float( delta.x() ), dy = float( delta.y() ), h = float( height() );

    bool changed = false;
    if ( event->buttons() & Qt::LeftButton )
    {
        // turn around what the camera looks at, or with a modifier move it
        if ( event->modifiers() & ( Qt::ShiftModifier | Qt::ControlModifier | Qt::MetaModifier ) )
        {
            changed = M_camera.pan( dx, dy, h );
        }
        else
        {
            changed = M_camera.orbit( dx, dy, h );
        }
    }
    else if ( event->buttons() & Qt::MiddleButton )
    {
        // down: farther, up: nearer, 5% per 100 pixels (as OrbitControls)
        changed = M_camera.dolly( float( std::pow( Cameras3D::DRAG_ZOOM, -dy * 0.01 ) ) );
    }
    else
    {
        QOpenGLWidget::mouseMoveEvent( event );
        return;
    }

    if ( changed )
    {
        emit cameraModeChanged( M_camera.mode() );
    }
    update();
}

/*-------------------------------------------------------------------*/
void
FieldView3D::viewFrom( const Viewpoint viewpoint )
{
    M_camera.viewFrom( viewpoint );
    update();
}

/*-------------------------------------------------------------------*/
RefereeTarget
FieldView3D::targetAt( const QPoint & point ) const
{
    if ( ! M_drawn )
    {
        return RefereeTarget();
    }
    return Picking::targetAt( M_drawn_camera, QSizeF( size() ), QPointF( point ), M_content.picks );
}

/*-------------------------------------------------------------------*/
void
FieldView3D::contextMenuEvent( QContextMenuEvent * event )
{
    // only a right click, as the 2D view (not the menu key)
    if ( event->reason() == QContextMenuEvent::Mouse )
    {
        emit refereeMenuRequested( targetAt( event->pos() ), event->globalPos() );
    }
}

/*-------------------------------------------------------------------*/
void
FieldView3D::drawNext()
{
    if ( M_moving )
    {
        update();
    }
}
