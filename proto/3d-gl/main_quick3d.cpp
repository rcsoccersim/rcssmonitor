// -*-c++-*-

/*
  A prototype of the 3D view (docs/PLAN_3D-VIEW.md, Phase 0, D1): Qt Quick
  3D in a QQuickWidget (or a QQuickView with --window). The figures are the
  model converted by balsam; their joints are moved every frame by the
  poses computed from the log (the same as the OpenGL prototype), through
  the properties of the joint nodes. Not part of the application.

  proto_quick3d [options] <game log>
    --window            a QQuickView instead of a QQuickWidget
    --no-vsync          do not wait for the vertical sync
    --bench <seconds>   measure for the seconds after 2 s, print, and quit
    --size <W>x<H>      the size of the drawing in device pixels (1920x1080)
    --cylinders         cylinders instead of the human figures
    --no-shadows        no shadow map
    --start <frame>     the first frame (1000)
    --screenshot <png>  save the drawing after 3 s, and quit
    --rotations-only    write only the rotations of the joints (and the
                        translation of the pelvis), not all their transforms
    --offscreen <n>     draw n frames into an offscreen texture with
                        QQuickRenderControl, without a window, and save the
                        last with --screenshot
*/

#include "gltf_rig.h"
#include "proto_log.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QImage>
#include <QMetaProperty>
#include <QPainter>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QQmlComponent>
#include <QQuickGraphicsDevice>
#include <QQuickRenderControl>
#include <QQuickRenderTarget>
#include <QQuickView>
#include <QQuickWidget>
#include <QQuickWindow>
#include <QScreen>
#include <QSurfaceFormat>
#include <QTemporaryDir>
#include <QTimer>
#include <QQuaternion>
#include <QVector3D>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <string>
#include <vector>

namespace {

const float FIGURE_HEIGHT = 1.85f * 1.2f;
const QVector3D SUN = QVector3D( -60.0f, 65.0f, -30.0f ).normalized();

double
mean( const std::vector< double > & v )
{
    return v.empty() ? 0.0 : std::accumulate( v.begin(), v.end(), 0.0 ) / v.size();
}

double
percentile( std::vector< double > v,
            double q )
{
    if ( v.empty() ) return 0.0;
    std::sort( v.begin(), v.end() );
    return v[std::min( v.size() - 1, std::size_t( q * ( v.size() - 1 ) + 0.5 ) )];
}

QString
lower( const QString & s )
{
    return s.toLower();
}

/*!
  \brief the pitch lines at 24 px/m on 140 m x 100 m, as the OpenGL prototype
*/
QImage
ground_image()
{
    const float W = 140.0f, H = 100.0f, PX = 24.0f;
    QImage image( int( W * PX ), int( H * PX ), QImage::Format_RGBA8888 );
    image.fill( QColor( 31, 120, 31 ) );
    QPainter p( &image );
    p.setRenderHint( QPainter::Antialiasing );
    p.translate( W * PX / 2, H * PX / 2 );
    p.scale( PX, PX );
    p.setPen( QPen( Qt::white, 0.12 ) );
    p.setBrush( Qt::NoBrush );
    p.drawRect( QRectF( -52.5, -34, 105, 68 ) );
    p.drawLine( QPointF( 0, -34 ), QPointF( 0, 34 ) );
    p.drawEllipse( QPointF( 0, 0 ), 9.15, 9.15 );
    for ( int side : { -1, 1 } )
    {
        p.drawRect( QRectF( side * 52.5 - ( side > 0 ? 16.5 : 0 ), -20.16, 16.5, 40.32 ) );
        p.drawRect( QRectF( side * 52.5 - ( side > 0 ? 5.5 : 0 ), -9.16, 5.5, 18.32 ) );
    }
    return image;
}

}

/*!
  \brief moves the scene every frame and measures it
*/
class Driver
    : public QObject {
public:
    const ProtoLog & log_;
    const GltfRig & rig_;
    int start_frame_ = 1000;
    double bench_seconds_ = 0.0;
    bool cylinders_ = false;
    bool shadows_ = true;
    bool rotations_only_ = false; //!< write only the rotations of the joints (and the translation of the pelvis)
    QQuickItem * root_ = nullptr;
    QQuickWindow * window_ = nullptr;
    std::function< QSize() > size_;

    Driver( const ProtoLog & log,
            const GltfRig & rig )
        : log_( log ),
          rig_( rig )
      { }

    struct Figure {
        QObject * root = nullptr;
        std::vector< QObject * > nodes; //!< by glTF node index, null if none
        QObject * cylinder = nullptr;
    };
    std::vector< Figure > figures_;
    QObject * camera_ = nullptr;
    QObject * ball_ = nullptr;
    QObject * view_ = nullptr;
    QMetaProperty p_position, p_rotation, p_scale, p_euler;

    QElapsedTimer clock_;
    qint64 last_frame_ns_ = -1;
    int frames_ = 0;
    double step_sum_ = 0.0;
    QElapsedTimer fps_clock_;
    QElapsedTimer bench_clock_;
    bool benching_ = false;
    std::vector< double > intervals_, steps_, render_ms_, sync_ms_;
    std::vector< RigTransform > local_;

    bool setup()
      {
          view_ = root_->findChild< QObject * >( "view" );
          camera_ = root_->findChild< QObject * >( "camera" );
          ball_ = root_->findChild< QObject * >( "ball" );
          QObject * sun = root_->findChild< QObject * >( "sun" );
          if ( ! view_ || ! camera_ || ! ball_ || ! sun )
          {
              std::fprintf( stderr, "the scene is not complete\n" );
              return false;
          }
          root_->setProperty( "figures", ! cylinders_ );
          root_->setProperty( "shadows", shadows_ );

          // a light shines along its -z (Node::lookAt is not invokable in Qt 6.4)
          sun->setProperty( "position", SUN * 100.0f );
          sun->setProperty( "rotation", QQuaternion::rotationTo( QVector3D( 0, 0, -1 ), -SUN ) );
          ball_->setProperty( "scale", QVector3D( 0.004f, 0.004f, 0.004f ) );

          const QMetaObject * mo = camera_->metaObject();
          p_position = mo->property( mo->indexOfProperty( "position" ) );
          p_rotation = mo->property( mo->indexOfProperty( "rotation" ) );
          p_scale = mo->property( mo->indexOfProperty( "scale" ) );
          p_euler = mo->property( mo->indexOfProperty( "eulerRotation" ) );

          const QVariantList figs = root_->property( "figureNodes" ).toList();
          const QVariantList cyls = root_->property( "cylinderNodes" ).toList();
          if ( figs.size() != PROTO_PLAYERS || cyls.size() != PROTO_PLAYERS )
          {
              std::fprintf( stderr, "the figures are not created\n" );
              return false;
          }

          // the joint nodes of a figure by their QML ids (balsam names them
          // after the glTF nodes, with the first letter in lower case)
          std::vector< QString > gltf_names;
          for ( const std::string & n : rig_.node_names_ ) gltf_names.push_back( lower( QString::fromStdString( n ) ) );

          int mapped = 0;
          for ( int i = 0; i < PROTO_PLAYERS; ++i )
          {
              Figure f;
              f.root = figs[i].value< QObject * >();
              f.cylinder = cyls[i].value< QObject * >();
              f.nodes.assign( rig_.node_names_.size(), nullptr );
              for ( QObject * child : f.root->findChildren< QObject * >() )
              {
                  QQmlContext * ctx = qmlContext( child );
                  if ( ! ctx ) continue;
                  const QString id = lower( ctx->nameForObject( child ) );
                  if ( id.isEmpty() ) continue;
                  for ( std::size_t n = 0; n < gltf_names.size(); ++n )
                  {
                      if ( gltf_names[n] == id && std::find( rig_.joint_nodes_.begin(), rig_.joint_nodes_.end(), int( n ) ) != rig_.joint_nodes_.end() )
                      {
                          f.nodes[n] = child;
                          if ( i == 0 ) ++mapped;
                      }
                  }
              }
              const bool left = ( i < 11 );
              const QColor color = ( left ? QColor::fromRgbF( 0.95f, 0.80f, 0.10f ) : QColor::fromRgbF( 0.15f, 0.55f, 0.95f ) );
              f.root->setProperty( "jersey", color );
              f.cylinder->setProperty( "color", color );
              f.root->setProperty( "scale", QVector3D( 1, 1, 1 ) * ( FIGURE_HEIGHT / rig_.height_ ) );
              f.cylinder->setProperty( "scale", QVector3D( 0.006f, 0.018f, 0.006f ) );
              figures_.push_back( f );
          }
          std::printf( "joint nodes found: %d of %d\n", mapped, rig_.jointCount() );
          std::fflush( stdout );
          return mapped == rig_.jointCount();
      }

    //! move everything to the time of now
    void step()
      {
          QElapsedTimer t;
          t.start();
          const double time = start_frame_ + clock_.nsecsElapsed() * 1e-9 / PROTO_STEP;

          static const std::vector< int > clips = { rig_.clipIndex( "Idle_Loop" ), rig_.clipIndex( "Walk_Loop" ),
                                                    rig_.clipIndex( "Jog_Fwd_Loop" ), rig_.clipIndex( "Sprint_Loop" ) };
          std::vector< float > times( 4 ), weights( 4 );
          const bool figures = ! cylinders_;
          for ( int i = 0; i < PROTO_PLAYERS; ++i )
          {
              const ProtoFigure fig = log_.figureAt( time, i );
              Figure & f = figures_[i];
              if ( figures )
              {
                  p_position.write( f.root, QVector3D( fig.x, 0.0f, fig.y ) );
                  p_euler.write( f.root, QVector3D( 0.0f, 90.0f - fig.body, 0.0f ) );
                  times.assign( fig.times, fig.times + 4 );
                  weights.assign( fig.weights, fig.weights + 4 );
                  rig_.localPose( clips, times, weights, local_ );
                  for ( std::size_t n = 0; n < f.nodes.size(); ++n )
                  {
                      if ( ! f.nodes[n] ) continue;
                      p_rotation.write( f.nodes[n], local_[n].r );
                      if ( rotations_only_ && rig_.node_names_[n] != "pelvis" ) continue;
                      p_position.write( f.nodes[n], local_[n].t );
                      p_scale.write( f.nodes[n], local_[n].s );
                  }
              }
              else
              {
                  p_position.write( f.cylinder, QVector3D( fig.x, 0.9f, fig.y ) );
              }
          }

          float bx, by;
          log_.ballAt( time, &bx, &by );
          p_position.write( ball_, QVector3D( bx, 0.2f, by ) );

          const QVector3D target( std::clamp( bx, -42.5f, 42.5f ), 0.0f, std::clamp( by, -24.0f, 24.0f ) );
          const float el = 32.0f * float( M_PI ) / 180.0f, az = 90.0f * float( M_PI ) / 180.0f;
          p_position.write( camera_, target + 72.0f * QVector3D( std::cos( el ) * std::cos( az ), std::sin( el ), std::cos( el ) * std::sin( az ) ) );
          QMetaObject::invokeMethod( camera_, "lookAt", Q_ARG( QVector3D, target ) );

          const double ms = t.nsecsElapsed() * 1e-6;
          step_sum_ += ms;
          if ( benching_ ) steps_.push_back( ms );
      }

    //! after a frame is drawn: measure, and move to the next
    void frameDone()
      {
          const qint64 now = clock_.nsecsElapsed();
          if ( benching_ && last_frame_ns_ >= 0 ) intervals_.push_back( ( now - last_frame_ns_ ) * 1e-6 );
          last_frame_ns_ = now;

          QObject * stats = view_->property( "renderStats" ).value< QObject * >();
          if ( benching_ && stats )
          {
              render_ms_.push_back( stats->property( "renderTime" ).toDouble() );
              sync_ms_.push_back( stats->property( "syncTime" ).toDouble() );
          }

          ++frames_;
          if ( fps_clock_.elapsed() >= 1000 )
          {
              const double s = fps_clock_.nsecsElapsed() * 1e-9;
              std::printf( "%.1f fps  poses+writes %.2f ms  render %.2f ms  sync %.2f ms  (%s, shadows %s)\n",
                           frames_ / s, step_sum_ / std::max( 1, frames_ ),
                           stats ? stats->property( "renderTime" ).toDouble() : -1.0,
                           stats ? stats->property( "syncTime" ).toDouble() : -1.0,
                           cylinders_ ? "cylinders" : "figures", shadows_ ? "on" : "off" );
              std::fflush( stdout );
              frames_ = 0;
              step_sum_ = 0.0;
              fps_clock_.restart();
          }

          if ( bench_seconds_ > 0.0 )
          {
              const double s = bench_clock_.nsecsElapsed() * 1e-9;
              if ( ! benching_ && s >= 2.0 ) benching_ = true;
              else if ( benching_ && s >= 2.0 + bench_seconds_ )
              {
                  const double interval = mean( intervals_ );
                  const QSize size = size_();
                  std::printf( "BENCH quick3d %s shadows=%s size=%dx%d frames=%zu fps=%.1f "
                               "interval_ms(mean=%.2f p50=%.2f p99=%.2f max=%.2f) poses+writes_ms(mean=%.2f p99=%.2f) "
                               "render_ms(mean=%.2f) sync_ms(mean=%.2f)\n",
                               cylinders_ ? "cylinders" : "figures", shadows_ ? "on" : "off",
                               size.width(), size.height(), intervals_.size(), interval > 0 ? 1000.0 / interval : 0.0,
                               interval, percentile( intervals_, 0.5 ), percentile( intervals_, 0.99 ), percentile( intervals_, 1.0 ),
                               mean( steps_ ), percentile( steps_, 0.99 ), mean( render_ms_ ), mean( sync_ms_ ) );
                  std::fflush( stdout );
                  QApplication::quit();
                  return;
              }
          }
          step();
          window_->update();
      }
};

/*!
  \brief draw frames with QQuickRenderControl into an OpenGL texture: to
  check the drawing where no window can be shown.
*/
int
render_offscreen( Driver & driver,
                  const QUrl & scene,
                  const QString & ground,
                  int width,
                  int height,
                  int frames,
                  const QString & image_path )
{
    QOpenGLContext context;
    context.setFormat( QSurfaceFormat::defaultFormat() );
    QOffscreenSurface surface;
    surface.setFormat( context.format() );
    surface.create();
    if ( ! context.create() || ! context.makeCurrent( &surface ) )
    {
        std::fprintf( stderr, "could not make an OpenGL context\n" );
        return 1;
    }

    QQuickRenderControl control;
    QQuickWindow window( &control );
    window.setGraphicsDevice( QQuickGraphicsDevice::fromOpenGLContext( &context ) );
    if ( ! control.initialize() )
    {
        std::fprintf( stderr, "could not initialize QQuickRenderControl\n" );
        return 1;
    }

    QQmlEngine engine;
    QQmlComponent component( &engine, scene );
    QQuickItem * root = qobject_cast< QQuickItem * >( component.create() );
    if ( ! root )
    {
        std::fprintf( stderr, "%s\n", qPrintable( component.errorString() ) );
        return 1;
    }
    root->setParentItem( window.contentItem() );
    root->setSize( QSizeF( width, height ) );
    window.resize( width, height );
    root->setProperty( "groundTexture", QUrl::fromLocalFile( ground ) );

    QOpenGLFunctions * gl = context.functions();
    GLuint texture = 0;
    gl->glGenTextures( 1, &texture );
    gl->glBindTexture( GL_TEXTURE_2D, texture );
    gl->glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr );
    window.setRenderTarget( QQuickRenderTarget::fromOpenGLTexture( texture, QSize( width, height ) ) );

    driver.root_ = root;
    driver.window_ = &window;
    driver.size_ = [width, height]() { return QSize( width, height ); };
    if ( ! driver.setup() )
    {
        return 1;
    }
    driver.clock_.start();

    std::vector< double > step_ms, frame_ms;
    for ( int i = 0; i < frames; ++i )
    {
        QElapsedTimer t;
        t.start();
        driver.step();
        step_ms.push_back( t.nsecsElapsed() * 1e-6 );
        QCoreApplication::processEvents();
        control.polishItems();
        control.beginFrame();
        control.sync();
        control.render();
        control.endFrame();
        gl->glFinish();
        frame_ms.push_back( t.nsecsElapsed() * 1e-6 );
    }
    std::printf( "OFFSCREEN quick3d %s shadows=%s size=%dx%d frames=%d poses+writes_ms(mean=%.2f) frame_ms(mean=%.2f p50=%.2f)\n",
                 driver.cylinders_ ? "cylinders" : "figures", driver.shadows_ ? "on" : "off",
                 width, height, frames, mean( step_ms ), mean( frame_ms ), percentile( frame_ms, 0.5 ) );

    int status = 0;
    if ( ! image_path.isEmpty() )
    {
        QImage image( width, height, QImage::Format_RGBA8888 );
        GLuint fbo = 0;
        gl->glGenFramebuffers( 1, &fbo );
        gl->glBindFramebuffer( GL_FRAMEBUFFER, fbo );
        gl->glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0 );
        gl->glReadPixels( 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, image.bits() );
        gl->glBindFramebuffer( GL_FRAMEBUFFER, 0 );
        gl->glDeleteFramebuffers( 1, &fbo );
        if ( ! image.mirrored().save( image_path ) ) status = 1;
    }
    delete root;
    gl->glDeleteTextures( 1, &texture );
    return status;
}

int
main( int argc,
      char ** argv )
{
    bool use_window = false, vsync = true, cylinders = false, shadows = true, rotations_only = false;
    int width = 1920, height = 1080, start = 1000;
    double bench = 0.0;
    int offscreen = 0;
    std::string log_path, screenshot;
    for ( int i = 1; i < argc; ++i )
    {
        const std::string a = argv[i];
        const bool more = ( i + 1 < argc );
        if ( a == "--window" ) use_window = true;
        else if ( a == "--no-vsync" ) vsync = false;
        else if ( a == "--bench" && more ) bench = std::atof( argv[++i] );
        else if ( a == "--size" && more ) std::sscanf( argv[++i], "%dx%d", &width, &height );
        else if ( a == "--cylinders" ) cylinders = true;
        else if ( a == "--no-shadows" ) shadows = false;
        else if ( a == "--rotations-only" ) rotations_only = true;
        else if ( a == "--start" && more ) start = std::atoi( argv[++i] );
        else if ( a == "--screenshot" && more ) screenshot = argv[++i];
        else if ( a == "--offscreen" && more ) offscreen = std::atoi( argv[++i] );
        else if ( a[0] != '-' ) log_path = a;
        else { std::fprintf( stderr, "unknown option %s\n", a.c_str() ); return 1; }
    }
    if ( log_path.empty() )
    {
        std::fprintf( stderr, "usage: %s [options] <game log>\n", argv[0] );
        return 1;
    }

    // OpenGL 3.3 core or later, as Qt Quick 3D wants it (its
    // idealSurfaceFormat() needs an application to probe OpenGL)
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setVersion( 3, 3 );
    format.setProfile( QSurfaceFormat::CoreProfile );
    format.setDepthBufferSize( 24 );
    format.setStencilBufferSize( 8 );
    format.setSwapInterval( vsync ? 1 : 0 );
    QSurfaceFormat::setDefaultFormat( format );
    QQuickWindow::setGraphicsApi( QSGRendererInterface::OpenGL );

    QApplication app( argc, argv );

    ProtoLog log;
    GltfRig rig;
    if ( ! log.load( log_path ) || ! rig.load( PROTO_MODEL_PATH ) )
    {
        return 1;
    }
    if ( start >= log.size() ) start = 0;
    std::printf( "Qt %s, %d frames, %s\n", qVersion(), log.size(), use_window ? "QQuickView" : "QQuickWidget" );

    QTemporaryDir tmp;
    const QString ground = tmp.filePath( "ground.png" );
    ground_image().save( ground );

    const QUrl scene = QUrl::fromLocalFile( QString( PROTO_QUICK3D_DIR ) + "/Scene.qml" );
    const qreal dpr = app.primaryScreen()->devicePixelRatio();
    const QSize logical( int( width / dpr + 0.5 ), int( height / dpr + 0.5 ) );

    Driver driver( log, rig );
    driver.start_frame_ = start;
    driver.bench_seconds_ = bench;
    driver.cylinders_ = cylinders;
    driver.shadows_ = shadows;
    driver.rotations_only_ = rotations_only;

    if ( offscreen > 0 )
    {
        return render_offscreen( driver, scene, ground, width, height, offscreen, QString::fromStdString( screenshot ) );
    }

    QQuickView * view = nullptr;
    QQuickWidget * widget = nullptr;
    if ( use_window )
    {
        view = new QQuickView;
        view->setResizeMode( QQuickView::SizeRootObjectToView );
        view->setSource( scene );
        view->resize( logical );
        view->setTitle( "3D prototype: Qt Quick 3D (QQuickView)" );
        driver.root_ = view->rootObject();
        driver.window_ = view;
        driver.size_ = [view]() { return view->size() * view->devicePixelRatio(); };
    }
    else
    {
        widget = new QQuickWidget;
        widget->setResizeMode( QQuickWidget::SizeRootObjectToView );
        widget->setSource( scene );
        widget->resize( logical );
        widget->setWindowTitle( "3D prototype: Qt Quick 3D (QQuickWidget)" );
        driver.root_ = widget->rootObject();
        driver.window_ = widget->quickWindow();
        driver.size_ = [widget]() { return widget->size() * widget->devicePixelRatioF(); };
    }
    if ( ! driver.root_ )
    {
        std::fprintf( stderr, "could not load %s\n", qPrintable( scene.toString() ) );
        return 1;
    }
    driver.root_->setProperty( "groundTexture", QUrl::fromLocalFile( ground ) );
    if ( ! driver.setup() )
    {
        return 1;
    }

    // a QQuickView signals each swapped frame; a QQuickWidget draws offscreen
    if ( view )
    {
        QObject::connect( view, &QQuickWindow::frameSwapped, &driver, &Driver::frameDone, Qt::QueuedConnection );
    }
    else
    {
        QObject::connect( widget->quickWindow(), &QQuickWindow::afterFrameEnd, &driver, &Driver::frameDone, Qt::QueuedConnection );
    }

    driver.clock_.start();
    driver.fps_clock_.start();
    driver.bench_clock_.start();
    driver.step();
    if ( view ) view->show();
    else widget->show();

    if ( ! screenshot.empty() )
    {
        QTimer::singleShot( 3000, [&]() {
            const QImage image = ( view ? view->grabWindow() : widget->grabFramebuffer() );
            image.save( QString::fromStdString( screenshot ) );
            QApplication::quit();
        } );
    }

    const int result = app.exec();
    delete view;
    delete widget;
    return result;
}
