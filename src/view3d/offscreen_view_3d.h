// -*-c++-*-

/*!
  \file offscreen_view_3d.h
  \brief the 3D view drawn into images, without a window. Header File.
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

#ifndef RCSSMONITOR_OFFSCREEN_VIEW_3D_H
#define RCSSMONITOR_OFFSCREEN_VIEW_3D_H

#include "cameras_3d.h"
#include "export_job.h"
#include "interpolate.h"
#include "view_3d_renderer.h"

#include <QImage>
#include <QSize>
#include <QString>

#include <cstddef>
#include <memory>

class DispHolder;
class QOffscreenSurface;
class QOpenGLContext;
class QOpenGLFramebufferObject;

/*!
  \class OffscreenView3D
  \brief the 3D view drawn into images (docs/plan/2026-11-22-03_frame-export.md
  4.2): an OpenGL context of its own on an offscreen surface, and a
  View3DRenderer of its own, so that the view on the screen is not
  changed. It needs OpenGL 3.3, not a window: it works under Xvfb and on
  the platform "offscreen".

  An export (ExportJob) draws a frame at the time of each image, between
  the frames of the log (ExportTiming::positions3D()), the camera moved
  for 1 / fps seconds of the video. The camera starts as the view's
  (start()), and jumps at the start of each range (startRange(),
  CameraController::snap()): the scenes of two ranges are apart, and the
  camera does not glide from one to the other. The motions of the players
  and the ball are found from the frames alone (FrameBuilder3D,
  BallSpin, BallFlight), the same however the frames are reached: nothing
  else starts again.

  Save Image of This Frame draws the frame shown with the view's camera as
  it is (renderStill()).

  The images are drawn into a framebuffer of their size (made again when
  it changes), read back, and the HUD is drawn over them with QPainter,
  as on the screen. The context is made by initialize(), once, and kept.
*/
class OffscreenView3D
    : public ExportRenderer {
public:

    //! the largest images of an export (the memory of the smooth edges)
    static constexpr int MAX_WIDTH = 3840;
    static constexpr int MAX_HEIGHT = 2160;

    explicit
    OffscreenView3D( DispHolder & disp_holder );
    ~OffscreenView3D() override;

    OffscreenView3D( const OffscreenView3D & ) = delete;
    OffscreenView3D & operator=( const OffscreenView3D & ) = delete;

    /*!
      \brief check if the 3D view can be drawn here: a context of OpenGL
      3.3 core with its functions (FieldView3D::unavailableReason() adds
      the platform, which must show windows)
      \return an empty string if it can, or why it cannot
    */
    static QString unavailableReason();

    /*!
      \brief make the context and the GPU's resources, once
      \param error set to the reason if not (may be null)
      \return true if the images can be drawn
    */
    bool initialize( QString * error = nullptr );

    //! initialize() succeeded
    bool ready() const { return M_ready; }

    /*!
      \brief start an export: the camera as given (the view's, or one of
      the camera of the options), moved 1 / fps seconds an image
    */
    void start( const CameraController & camera,
                const double fps );

    //! a range starts: the camera jumps to where it goes (CameraController::snap())
    void startRange() override;

    //! the image i of a part, at its position (ExportPart::positions; the frame without one)
    QImage render( const ExportPart & part,
                   const std::size_t i,
                   const QSize & pixels,
                   const qreal dpr ) override;

    /*!
      \brief draw at a position between the frames, the camera of the export
      moved dt seconds
      \param position the frame, and the fraction to the next (Interpolate::positionAt())
      \param pixels the size of the image
      \param dpr the device pixel ratio: the view's layout is pixels / dpr
      \param dt the time since the image before (s)
      \return the image (Format_RGB32), or a null image
    */
    QImage renderAt( const double position,
                     const QSize & pixels,
                     const qreal dpr,
                     const double dt );

    /*!
      \brief draw a frame with a camera as it is, not moved (Save Image of
      This Frame: the view's camera)
    */
    QImage renderStill( const std::size_t index,
                        const CameraController & camera,
                        const QSize & pixels,
                        const qreal dpr );

    //! the camera of the export
    const CameraController & camera() const { return M_camera; }

    //! what the last image drew (the tests)
    View3DState state() const;

private:

    DispHolder & M_disp_holder;
    std::unique_ptr< QOpenGLContext > M_context;
    std::unique_ptr< QOffscreenSurface > M_surface;
    std::unique_ptr< QOpenGLFramebufferObject > M_fbo;
    View3DRenderer M_renderer;
    CameraController M_camera; //!< of the export
    double M_dt; //!< 1 / fps of the export (s)
    bool M_ready;
    CameraPose M_drawn_camera; //!< of the last image
    bool M_drawn;

    //! draw at a position with a camera moved dt seconds
    QImage draw( const double position,
                 CameraController & camera,
                 const double dt,
                 const QSize & pixels,
                 const qreal dpr );
};

#endif
