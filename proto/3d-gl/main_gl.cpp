// -*-c++-*-

/*
  A prototype of the 3D view (docs/PLAN_3D-VIEW.md, Phase 0, D1): OpenGL
  3.3 core in a QOpenGLWidget. Not part of the application.

  proto_gl [options] <game log>
    --no-vsync          do not wait for the vertical sync (measure the headroom)
    --bench <seconds>   measure for the seconds after 2 s, print, and quit
    --size <W>x<H>      the size of the drawing in device pixels (1920x1080)
    --cylinders         cylinders instead of the human figures
    --no-shadows        no shadow map
    --shadow-size <n>   the size of the shadow map (4096)
    --start <frame>     the first frame (1000)
    --screenshot <png>  save the drawing after 2 s, and quit
    --offscreen <n>     draw n frames into an offscreen framebuffer, without
                        a window, and save the last with --screenshot
*/

#include "view_gl.h"

#include <QApplication>
#include <QScreen>
#include <QSurfaceFormat>
#include <QTimer>

#include <cstdio>
#include <cstring>
#include <string>

int
main( int argc,
      char ** argv )
{
    ProtoOptions options;
    bool vsync = true;
    int width = 1920, height = 1080;
    int offscreen_frames = 0;
    std::string log_path, screenshot;
    for ( int i = 1; i < argc; ++i )
    {
        const std::string a = argv[i];
        const bool more = ( i + 1 < argc );
        if ( a == "--no-vsync" ) vsync = false;
        else if ( a == "--bench" && more ) options.bench_seconds = std::atof( argv[++i] );
        else if ( a == "--size" && more ) std::sscanf( argv[++i], "%dx%d", &width, &height );
        else if ( a == "--cylinders" ) options.figures = false;
        else if ( a == "--no-shadows" ) options.shadows = false;
        else if ( a == "--shadow-size" && more ) options.shadow_size = std::atoi( argv[++i] );
        else if ( a == "--start" && more ) options.start_frame = std::atoi( argv[++i] );
        else if ( a == "--screenshot" && more ) screenshot = argv[++i];
        else if ( a == "--offscreen" && more ) offscreen_frames = std::atoi( argv[++i] );
        else if ( a[0] != '-' ) log_path = a;
        else { std::fprintf( stderr, "unknown option %s\n", a.c_str() ); return 1; }
    }
    if ( log_path.empty() )
    {
        std::fprintf( stderr, "usage: %s [options] <game log>\n", argv[0] );
        return 1;
    }

#if QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
    QApplication::setAttribute( Qt::AA_EnableHighDpiScaling );
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy( Qt::HighDpiScaleFactorRoundingPolicy::PassThrough );
#endif
    QSurfaceFormat format;
    format.setVersion( 3, 3 );
    format.setProfile( QSurfaceFormat::CoreProfile );
    format.setDepthBufferSize( 24 );
    format.setSamples( 4 );
    format.setSwapInterval( vsync ? 1 : 0 );
    QSurfaceFormat::setDefaultFormat( format );

    QApplication app( argc, argv );

    ProtoLog log;
    GltfRig rig;
    if ( ! log.load( log_path ) || ! rig.load( PROTO_MODEL_PATH ) )
    {
        return 1;
    }
    std::printf( "Qt %s, %d frames, model: %zu vertices, %zu triangles, %d joints, %zu clips\n",
                 qVersion(), log.size(), rig.vertices_.size(), rig.indices_.size() / 3, rig.jointCount(), rig.clips_.size() );
    if ( options.start_frame >= log.size() ) options.start_frame = 0;

    if ( offscreen_frames > 0 )
    {
        return render_offscreen( log, rig, options, width, height, offscreen_frames,
                                 QString::fromStdString( screenshot ) );
    }

    ViewGL view( log, rig, options );
    view.setWindowTitle( "3D prototype: QOpenGLWidget" );
    const qreal dpr = app.primaryScreen()->devicePixelRatio();
    view.resize( int( width / dpr + 0.5 ), int( height / dpr + 0.5 ) );
    view.show();

    if ( ! screenshot.empty() )
    {
        QTimer::singleShot( 2000, [&]() {
            view.grabFramebuffer().save( QString::fromStdString( screenshot ) );
            QApplication::quit();
        } );
    }

    return app.exec();
}
