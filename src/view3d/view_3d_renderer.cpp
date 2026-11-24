// -*-c++-*-

/*!
  \file view_3d_renderer.cpp
  \brief what the 3D view draws in a frame, apart from its window. Source File.
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

#include "view_3d_renderer.h"

#include "ground_texture.h"
#include "guides_3d.h"
#include "overlay_builder.h"
#include "picking.h"
#include "scene_builder.h"
#include "stadium_3d.h"
#include "stands_3d.h"

#include "disp_holder.h"
#include "field_painter.h"
#include "options.h"
#include "player_selection.h"
#include "score_board_painter.h"
#include "team_graphic_painter.h"

#include <QElapsedTimer>
#include <QFontInfo>
#include <QFontMetrics>
#include <QMatrix4x4>
#include <QPainter>

#include <algorithm>
#include <cmath>
#include <iostream>

/*-------------------------------------------------------------------*/
View3DRenderer::View3DRenderer( DispHolder & disp_holder )
    : M_disp_holder( disp_holder ),
      M_field_painter( new FieldPainter( disp_holder ) ),
      M_score_board_painter( new ScoreBoardPainter( disp_holder ) ),
      M_team_graphic_painter( new TeamGraphicPainter( disp_holder ) ),
      M_ready( false ),
      M_build_time( 0.0 ),
      M_submit_time( 0.0 )
{

}

/*-------------------------------------------------------------------*/
View3DRenderer::~View3DRenderer()
{
    // the painters are complete here (std::unique_ptr)
}

/*-------------------------------------------------------------------*/
bool
View3DRenderer::initialize()
{
    M_ready = M_scene.initialize();
    if ( ! M_ready )
    {
        std::cerr << "(FieldView3D) OpenGL 3.3 core is not available" << std::endl;
    }
    // the human figures, read once (FrameBuilder3D), their body sent to
    // every new context when drawn (build())
    else if ( ! M_builder.figures().load() )
    {
        std::cerr << "(FieldView3D) the human figures are not available: "
                  << M_builder.figures().error().toStdString() << std::endl;
    }
    M_figure_pack.clear();
    M_ground_key.clear();
    M_stadium_keys[0].clear();
    M_stadium_keys[1].clear();
    M_stands_key.clear();
    // the new context has no texture of the labels yet
    M_label_atlas = LabelAtlas();
    return M_ready;
}

/*-------------------------------------------------------------------*/
void
View3DRenderer::release()
{
    M_scene.release();
}

/*-------------------------------------------------------------------*/
void
View3DRenderer::updateImages()
{
    // the ground image: drawn again only when its colors or options changed
    const QString key = GroundTexture::key( M_disp_holder.serverParam() );
    if ( key != M_ground_key )
    {
        M_scene.setGround( GroundTexture::draw( *M_field_painter ) );
        M_ground_key = key;
    }

    // The ends in the team colors: the boards and the banners with the team
    // names and logos, drawn again when they change; and the stands, with
    // the fans of both. The logos as the frame has them, before the HUD
    // draws them (the first frame has them on the boards too).
    const Options & opt = Options::instance();
    M_team_graphic_painter->update();
    const DispConstPtr current = M_disp_holder.currentDisp();
    Stadium3D::TeamLook looks[2];
    for ( int end = 0; end < 2; ++end )
    {
        Stadium3D::TeamLook & look = looks[end];
        look.color = ( end == 0 ? opt.leftTeamBrush() : opt.rightTeamBrush() ).color();
        look.name = ( current ? QString::fromStdString( current->team_[end].name_ ) : QString() );
        if ( opt.showTeamGraphic() )
        {
            look.logo = M_team_graphic_painter->pixmap( end == 0 ).toImage();
        }
        const QString look_key = look.key();
        if ( look_key != M_stadium_keys[end] )
        {
            M_scene.setStadium( end, Stadium3D::boardImage( look ), Stadium3D::bannerImage( look ) );
            M_stadium_keys[end] = look_key;
        }
    }
    // the stands only need the colors and the names (no logos)
    const QString stands_key = looks[0].color.name() + looks[0].name + '|' + looks[1].color.name() + looks[1].name;
    if ( opt.stands3D() && stands_key != M_stands_key )
    {
        for ( int i = 0; i < Stands3D::STAND_COUNT; ++i )
        {
            M_scene.setStands( i, Stands3D::image( i, looks[0], looks[1] ) );
        }
        M_stands_key = stands_key;
    }
}

/*-------------------------------------------------------------------*/
void
View3DRenderer::build( const Interpolate::Position & at,
                       const bool live,
                       const std::size_t delay_frames,
                       const CameraMode mode )
{
    // the automatic selection of a player, as the 2D view does when it is drawn
    if ( const DispConstPtr current = M_disp_holder.currentDisp() )
    {
        selectPlayerNearestBall( current->show_, Options::instance() );
    }

    updateImages();

    QElapsedTimer cpu;
    cpu.start();

    // what to draw where (FrameBuilder3D): the goals and the boards, the
    // players (posed from the log, as figures) and the ball on the way
    // between the two frames, the overlays
    const Options & opt = Options::instance();
    M_builder.setLive( live, delay_frames ); // the ball flies virtually live only when delayed
    M_builder.build( M_disp_holder, opt, at, mode, M_content );
    // the bodies of the figures to the GPU, when others are drawn
    if ( ! M_content.figures.empty()
         && M_builder.figures().pack().dir != M_figure_pack )
    {
        const Figures3D & figures = M_builder.figures();
        for ( int k = 0; k < figures.bodyCount(); ++k )
        {
            M_scene.setFigureBody( k, figures.model( k ), figures.paints( k ), figures.uniformCuts( k ) );
        }
        M_figure_pack = figures.pack().dir;
    }

    M_build_time = cpu.nsecsElapsed() * 1.0e-6;
}

/*-------------------------------------------------------------------*/
void
View3DRenderer::render( const GLuint fbo,
                        const CameraPose & camera,
                        const QSize & logical,
                        const qreal dpr,
                        const int samples )
{
    QElapsedTimer cpu;
    cpu.start();

    // the number labels and the cards, sized for the camera and the screen
    const Options & opt = Options::instance();
    if ( ! M_disp_holder.dispCont().empty() )
    {
        const SceneBuilder::LabelView label_view = {
            camera,
            float( logical.height() ),
            double( QFontInfo( opt.playerFont() ).pixelSize() ),
        };
        SceneBuilder::addLabels( M_content, M_builder.show(), M_disp_holder, opt, M_label_atlas, label_view, M_builder.seen() );
        // the ring around the ball and its trail, a few pixels wide at least
        M_builder.addBallAids( M_disp_holder, opt, camera, float( logical.height() ), M_content );
    }
    if ( M_label_atlas.takeChanged() )
    {
        M_scene.setLabels( M_label_atlas.image() );
    }

    M_build_time += cpu.nsecsElapsed() * 1.0e-6;

    // draw into the framebuffer, in physical pixels: on a screen scaled by
    // 1.5, a view of 100 logical pixels has 150 of them
    cpu.restart();
    M_scene.render( fbo,
                    static_cast< int >( std::lround( logical.width() * dpr ) ),
                    static_cast< int >( std::lround( logical.height() * dpr ) ),
                    camera,
                    M_content,
                    samples );
    M_submit_time = cpu.nsecsElapsed() * 1.0e-6;
}

/*-------------------------------------------------------------------*/
bool
View3DRenderer::minimapShown( const CameraMode mode )
{
    return Guides3D::shown( Options::instance().minimap3D(), mode );
}

/*-------------------------------------------------------------------*/
void
View3DRenderer::drawHud( QPainter & painter,
                         const CameraPose & camera,
                         const QSize & logical,
                         const qreal dpr,
                         const CameraMode mode )
{
    const Options & opt = Options::instance();
    const int width = logical.width();
    const int height = logical.height();
    const float aspect = float( width ) / float( std::max( 1, height ) );

    // The team logo on the right is placed by the size of the canvas in the
    // options, which the 2D view sets when it is drawn: set it here, the
    // same size.
    Options::instance().updateFieldSize( width, height );
    M_score_board_painter->draw( painter );
    M_team_graphic_painter->draw( painter );

    // the texts of the overlays, beside their points, in the player font
    if ( ! M_content.texts.empty() )
    {
        const QFont & font = opt.playerFont();
        painter.setFont( font );
        const QMatrix4x4 view_projection = camera.projection( aspect ) * camera.view();
        for ( const OverlayBuilder::PlacedText & t : OverlayBuilder::placeTexts( M_content.texts, view_projection, logical, QFontMetrics( font ) ) )
        {
            painter.setPen( t.color );
            painter.drawText( t.baseline, t.text );
        }
    }

    // the guides: the compass at the top, the minimap at the bottom right
    const bool compass = Guides3D::shown( opt.compass3D(), mode );
    const bool minimap = minimapShown( mode );
    if ( compass || minimap )
    {
        const rcss::rcg::ShowInfoT & show = M_builder.show();
        const Guides3D::CameraGuide guide = Guides3D::guideOf( camera, aspect );
        if ( compass )
        {
            const QPointF ball( show.ball_.x_, show.ball_.y_ );
            // the ball as the camera's player perceives it
            const Perceptions * seen = M_builder.seen();
            const bool ball_seen = ( ! seen || seen->ball );
            Guides3D::drawCompass( painter, Guides3D::compassRect( width ), guide,
                                   ball_seen ? &ball : nullptr, opt );
        }
        if ( minimap )
        {
            double scale = 1.0;
            const QRectF rect = Guides3D::minimapRect( width, height, &scale );
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

/*-------------------------------------------------------------------*/
View3DState
View3DRenderer::state( const CameraPose & camera ) const
{
    return M_builder.state( M_content, camera );
}

/*-------------------------------------------------------------------*/
RefereeTarget
View3DRenderer::targetAt( const CameraPose & camera,
                          const QSizeF & size,
                          const QPointF & point ) const
{
    return Picking::targetAt( camera, size, point, M_content.picks );
}
