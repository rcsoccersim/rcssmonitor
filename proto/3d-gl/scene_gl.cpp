// -*-c++-*-

/*
  A prototype of the 3D view (docs/PLAN_3D-VIEW.md, Phase 0, D1). Not part
  of the application.
*/

#include "scene_gl.h"

#include <QElapsedTimer>
#include <QImage>
#include <QOpenGLShaderProgram>
#include <QPainter>

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {

const float FIGURE_SCALE = 1.2f;
const float FIGURE_HEIGHT = 1.85f * FIGURE_SCALE;
const QVector3D SUN = QVector3D( -60.0f, 65.0f, -30.0f ).normalized(); // toward the sun
const int INSTANCE_FLOATS = 20; // mat4 + color

const char * LIGHTING = R"(
uniform sampler2DShadow uShadow;
uniform int uShadows;
uniform float uShadowTexel;
uniform vec3 uSun;
float shadowAt( vec4 lp )
{
    if ( uShadows == 0 ) return 1.0;
    vec3 p = lp.xyz / lp.w * 0.5 + 0.5;
    if ( p.z >= 1.0 || p.x < 0.0 || p.x > 1.0 || p.y < 0.0 || p.y > 1.0 ) return 1.0;
    float s = 0.0;
    for ( int y = -2; y <= 2; ++y )
        for ( int x = -2; x <= 2; ++x )
            s += texture( uShadow, vec3( p.xy + vec2( x, y ) * uShadowTexel, p.z - 0.0005 ) );
    return s / 25.0;
}
vec3 lit( vec3 color, vec3 normal, vec4 lp )
{
    float d = max( dot( normalize( normal ), uSun ), 0.0 );
    float sh = shadowAt( lp );
    return color * ( 0.45 + 0.65 * d * sh );
}
)";

const char * SKINNED_VS = R"(#version 330 core
layout( location = 0 ) in vec3 aPos;
layout( location = 1 ) in vec3 aNormal;
layout( location = 2 ) in uvec4 aJoints;
layout( location = 3 ) in vec4 aWeights;
uniform sampler2D uJoints;
uniform mat4 uViewProj;
uniform mat4 uLightViewProj;
uniform vec3 uColors[22];
uniform int uJersey;
out vec3 vNormal;
out vec4 vLightPos;
out vec3 vColor;
mat4 jointMatrix( uint j )
{
    int x = int( j ) * 4;
    int y = gl_InstanceID;
    return mat4( texelFetch( uJoints, ivec2( x, y ), 0 ),
                 texelFetch( uJoints, ivec2( x + 1, y ), 0 ),
                 texelFetch( uJoints, ivec2( x + 2, y ), 0 ),
                 texelFetch( uJoints, ivec2( x + 3, y ), 0 ) );
}
void main()
{
    mat4 m = aWeights.x * jointMatrix( aJoints.x ) + aWeights.y * jointMatrix( aJoints.y )
           + aWeights.z * jointMatrix( aJoints.z ) + aWeights.w * jointMatrix( aJoints.w );
    vec4 wp = m * vec4( aPos, 1.0 );
    vNormal = mat3( m ) * aNormal;
    vLightPos = uLightViewProj * ( wp + vec4( normalize( vNormal ) * 0.02, 0.0 ) );
    vColor = ( uJersey != 0 ? uColors[gl_InstanceID] : vec3( 0.25 ) );
    gl_Position = uViewProj * wp;
}
)";

const char * INSTANCED_VS = R"(#version 330 core
layout( location = 0 ) in vec3 aPos;
layout( location = 1 ) in vec3 aNormal;
layout( location = 4 ) in mat4 aModel;
layout( location = 8 ) in vec4 aColor;
uniform mat4 uViewProj;
uniform mat4 uLightViewProj;
out vec3 vNormal;
out vec4 vLightPos;
out vec3 vColor;
void main()
{
    vec4 wp = aModel * vec4( aPos, 1.0 );
    vNormal = mat3( aModel ) * aNormal;
    vLightPos = uLightViewProj * ( wp + vec4( normalize( vNormal ) * 0.02, 0.0 ) );
    vColor = aColor.rgb;
    gl_Position = uViewProj * wp;
}
)";

const char * LIT_FS_HEAD = R"(#version 330 core
in vec3 vNormal;
in vec4 vLightPos;
in vec3 vColor;
out vec4 fragColor;
)";

const char * LIT_FS_MAIN = R"(
void main()
{
    fragColor = vec4( lit( vColor, vNormal, vLightPos ), 1.0 );
}
)";

const char * DEPTH_FS = R"(#version 330 core
void main() {}
)";

const char * GROUND_VS = R"(#version 330 core
layout( location = 0 ) in vec3 aPos;
layout( location = 1 ) in vec2 aUV;
uniform mat4 uViewProj;
uniform mat4 uLightViewProj;
out vec2 vUV;
out vec4 vLightPos;
void main()
{
    vUV = aUV;
    vLightPos = uLightViewProj * vec4( aPos, 1.0 );
    gl_Position = uViewProj * vec4( aPos, 1.0 );
}
)";

const char * GROUND_FS_HEAD = R"(#version 330 core
in vec2 vUV;
in vec4 vLightPos;
uniform sampler2D uTexture;
out vec4 fragColor;
)";

const char * GROUND_FS_MAIN = R"(
void main()
{
    vec3 c = texture( uTexture, vUV ).rgb;
    fragColor = vec4( c * ( 0.6 + 0.4 * shadowAt( vLightPos ) ), 1.0 );
}
)";

std::unique_ptr< QOpenGLShaderProgram >
make_program( const QString & vs,
              const QString & fs )
{
    std::unique_ptr< QOpenGLShaderProgram > p( new QOpenGLShaderProgram );
    if ( ! p->addShaderFromSourceCode( QOpenGLShader::Vertex, vs )
         || ! p->addShaderFromSourceCode( QOpenGLShader::Fragment, fs )
         || ! p->link() )
    {
        std::fprintf( stderr, "shader error: %s\n", qPrintable( p->log() ) );
    }
    return p;
}

QMatrix4x4
light_view_proj()
{
    QMatrix4x4 view;
    view.lookAt( SUN * 100.0f, QVector3D( 0, 0, 0 ), QVector3D( 0, 1, 0 ) );
    float min_x = 1e9f, max_x = -1e9f, min_y = 1e9f, max_y = -1e9f, min_z = 1e9f, max_z = -1e9f;
    for ( float x : { -74.0f, 74.0f } )
        for ( float y : { 0.0f, 3.0f } )
            for ( float z : { -54.0f, 54.0f } )
            {
                const QVector3D p = view.map( QVector3D( x, y, z ) );
                min_x = std::min( min_x, p.x() ); max_x = std::max( max_x, p.x() );
                min_y = std::min( min_y, p.y() ); max_y = std::max( max_y, p.y() );
                min_z = std::min( min_z, p.z() ); max_z = std::max( max_z, p.z() );
            }
    QMatrix4x4 proj;
    proj.ortho( min_x, max_x, min_y, max_y, -max_z - 1.0f, -min_z + 1.0f );
    return proj * view;
}


}

/*-------------------------------------------------------------------*/
SceneGL::SceneGL( const ProtoLog & log,
                  const GltfRig & rig,
                  const ProtoOptions & options )
    : options_( options ),
      M_log( log ),
      M_rig( rig )
{
    for ( int i = 0; i < PROTO_PLAYERS; ++i )
    {
        const bool left = ( i < 11 );
        M_colors[i * 3 + 0] = ( left ? 0.95f : 0.15f );
        M_colors[i * 3 + 1] = ( left ? 0.80f : 0.55f );
        M_colors[i * 3 + 2] = ( left ? 0.10f : 0.95f );
    }
}

/*-------------------------------------------------------------------*/
SceneGL::~SceneGL() = default;

/*-------------------------------------------------------------------*/
bool
SceneGL::initialize()
{
    if ( ! initializeOpenGLFunctions() )
    {
        std::fprintf( stderr, "OpenGL 3.3 core is not available\n" );
        return false;
    }
    std::printf( "GL_VENDOR %s\nGL_RENDERER %s\nGL_VERSION %s\n",
                 reinterpret_cast< const char * >( glGetString( GL_VENDOR ) ),
                 reinterpret_cast< const char * >( glGetString( GL_RENDERER ) ),
                 reinterpret_cast< const char * >( glGetString( GL_VERSION ) ) );
    std::fflush( stdout );

    const QString lit_fs = QString( LIT_FS_HEAD ) + LIGHTING + LIT_FS_MAIN;
    M_skinned = make_program( SKINNED_VS, lit_fs );
    M_skinned_depth = make_program( SKINNED_VS, DEPTH_FS );
    M_instanced = make_program( INSTANCED_VS, lit_fs );
    M_instanced_depth = make_program( INSTANCED_VS, DEPTH_FS );
    M_ground = make_program( GROUND_VS, QString( GROUND_FS_HEAD ) + LIGHTING + GROUND_FS_MAIN );

    createRig();
    createCylinder();
    createSphere();
    createGround();
    createShadowMap();
    glGenQueries( 2, M_queries );
    return M_skinned->isLinked() && M_instanced->isLinked() && M_ground->isLinked();
}

/*-------------------------------------------------------------------*/
void
SceneGL::release()
{
    GLuint vaos[] = { M_rig_vao, M_cyl_vao, M_sphere_vao, M_ground_vao };
    glDeleteVertexArrays( 4, vaos );
    GLuint bufs[] = { M_rig_vbo, M_rig_ibo, M_cyl_vbo, M_cyl_ibo, M_cyl_inst, M_sphere_vbo, M_sphere_ibo, M_sphere_inst, M_ground_vbo };
    glDeleteBuffers( 9, bufs );
    GLuint texs[] = { M_ground_tex, M_joint_tex, M_shadow_tex };
    glDeleteTextures( 3, texs );
    glDeleteFramebuffers( 1, &M_shadow_fbo );
    glDeleteQueries( 2, M_queries );
    M_skinned.reset();
    M_skinned_depth.reset();
    M_instanced.reset();
    M_instanced_depth.reset();
    M_ground.reset();
}

/*-------------------------------------------------------------------*/
void
SceneGL::createRig()
{
    glGenVertexArrays( 1, &M_rig_vao );
    glBindVertexArray( M_rig_vao );
    glGenBuffers( 1, &M_rig_vbo );
    glBindBuffer( GL_ARRAY_BUFFER, M_rig_vbo );
    glBufferData( GL_ARRAY_BUFFER, M_rig.vertices_.size() * sizeof( RigVertex ), M_rig.vertices_.data(), GL_STATIC_DRAW );
    glGenBuffers( 1, &M_rig_ibo );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, M_rig_ibo );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, M_rig.indices_.size() * sizeof( std::uint32_t ), M_rig.indices_.data(), GL_STATIC_DRAW );
    const GLsizei stride = sizeof( RigVertex );
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast< void * >( offsetof( RigVertex, pos ) ) );
    glEnableVertexAttribArray( 1 );
    glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast< void * >( offsetof( RigVertex, normal ) ) );
    glEnableVertexAttribArray( 2 );
    glVertexAttribIPointer( 2, 4, GL_UNSIGNED_BYTE, stride, reinterpret_cast< void * >( offsetof( RigVertex, joints ) ) );
    glEnableVertexAttribArray( 3 );
    glVertexAttribPointer( 3, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast< void * >( offsetof( RigVertex, weights ) ) );
    glBindVertexArray( 0 );

    // the skin matrices: 4 texels a matrix, a row a player
    glGenTextures( 1, &M_joint_tex );
    glBindTexture( GL_TEXTURE_2D, M_joint_tex );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, M_rig.jointCount() * 4, PROTO_PLAYERS, 0, GL_RGBA, GL_FLOAT, nullptr );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    M_joint_data.assign( PROTO_PLAYERS * M_rig.jointCount() * 16, 0.0f );
}

/*-------------------------------------------------------------------*/
namespace {

void
setup_instances( QOpenGLFunctions_3_3_Core * gl,
                 GLuint * inst )
{
    gl->glGenBuffers( 1, inst );
    gl->glBindBuffer( GL_ARRAY_BUFFER, *inst );
    const GLsizei stride = INSTANCE_FLOATS * sizeof( float );
    for ( int c = 0; c < 4; ++c )
    {
        gl->glEnableVertexAttribArray( 4 + c );
        gl->glVertexAttribPointer( 4 + c, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast< void * >( c * 4 * sizeof( float ) ) );
        gl->glVertexAttribDivisor( 4 + c, 1 );
    }
    gl->glEnableVertexAttribArray( 8 );
    gl->glVertexAttribPointer( 8, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast< void * >( 16 * sizeof( float ) ) );
    gl->glVertexAttribDivisor( 8, 1 );
}

void
upload_mesh( QOpenGLFunctions_3_3_Core * gl,
             const std::vector< float > & vertices, // pos, normal
             const std::vector< std::uint32_t > & indices,
             GLuint * vao, GLuint * vbo, GLuint * ibo )
{
    gl->glGenVertexArrays( 1, vao );
    gl->glBindVertexArray( *vao );
    gl->glGenBuffers( 1, vbo );
    gl->glBindBuffer( GL_ARRAY_BUFFER, *vbo );
    gl->glBufferData( GL_ARRAY_BUFFER, vertices.size() * sizeof( float ), vertices.data(), GL_STATIC_DRAW );
    gl->glGenBuffers( 1, ibo );
    gl->glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, *ibo );
    gl->glBufferData( GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof( std::uint32_t ), indices.data(), GL_STATIC_DRAW );
    gl->glEnableVertexAttribArray( 0 );
    gl->glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof( float ), nullptr );
    gl->glEnableVertexAttribArray( 1 );
    gl->glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof( float ), reinterpret_cast< void * >( 3 * sizeof( float ) ) );
}

}

/*-------------------------------------------------------------------*/
void
SceneGL::createCylinder()
{
    // radius 1, from y = 0 to 1, with caps
    const int n = 24;
    std::vector< float > v;
    std::vector< std::uint32_t > idx;
    for ( int i = 0; i <= n; ++i )
    {
        const float a = 2.0f * float( M_PI ) * i / n;
        const float c = std::cos( a ), s = std::sin( a );
        for ( float y : { 0.0f, 1.0f } )
        {
            v.insert( v.end(), { c, y, s, c, 0.0f, s } );
        }
    }
    for ( int i = 0; i < n; ++i )
    {
        const std::uint32_t a = i * 2;
        idx.insert( idx.end(), { a, a + 1, a + 2, a + 1, a + 3, a + 2 } );
    }
    const std::uint32_t top = static_cast< std::uint32_t >( v.size() / 6 );
    v.insert( v.end(), { 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f } );
    for ( int i = 0; i <= n; ++i )
    {
        const float a = 2.0f * float( M_PI ) * i / n;
        v.insert( v.end(), { std::cos( a ), 1.0f, std::sin( a ), 0.0f, 1.0f, 0.0f } );
    }
    for ( int i = 0; i < n; ++i )
    {
        idx.insert( idx.end(), { top, top + 2 + i, top + 1 + i } );
    }
    upload_mesh( this, v, idx, &M_cyl_vao, &M_cyl_vbo, &M_cyl_ibo );
    setup_instances( this, &M_cyl_inst );
    glBindVertexArray( 0 );
    M_cyl_count = static_cast< GLsizei >( idx.size() );
    M_cyl_data.assign( PROTO_PLAYERS * INSTANCE_FLOATS, 0.0f );
}

/*-------------------------------------------------------------------*/
void
SceneGL::createSphere()
{
    const int rings = 12, segs = 16;
    std::vector< float > v;
    std::vector< std::uint32_t > idx;
    for ( int r = 0; r <= rings; ++r )
    {
        const float t = float( M_PI ) * r / rings;
        for ( int s = 0; s <= segs; ++s )
        {
            const float p = 2.0f * float( M_PI ) * s / segs;
            const float x = std::sin( t ) * std::cos( p ), y = std::cos( t ), z = std::sin( t ) * std::sin( p );
            v.insert( v.end(), { x, y, z, x, y, z } );
        }
    }
    for ( int r = 0; r < rings; ++r )
    {
        for ( int s = 0; s < segs; ++s )
        {
            const std::uint32_t a = r * ( segs + 1 ) + s, b = a + segs + 1;
            idx.insert( idx.end(), { a, b, a + 1, a + 1, b, b + 1 } );
        }
    }
    upload_mesh( this, v, idx, &M_sphere_vao, &M_sphere_vbo, &M_sphere_ibo );
    setup_instances( this, &M_sphere_inst );
    glBindVertexArray( 0 );
    M_sphere_count = static_cast< GLsizei >( idx.size() );
    M_sphere_data.assign( INSTANCE_FLOATS, 0.0f );
}

/*-------------------------------------------------------------------*/
void
SceneGL::createGround()
{
    // 140 m x 100 m, the pitch lines drawn with QPainter at 24 px/m
    const float W = 140.0f, H = 100.0f, PX = 24.0f;
    QImage image( int( W * PX ), int( H * PX ), QImage::Format_RGBA8888 );
    image.fill( QColor( 31, 120, 31 ) );
    {
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
    }
    glGenTextures( 1, &M_ground_tex );
    glBindTexture( GL_TEXTURE_2D, M_ground_tex );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.constBits() );
    glGenerateMipmap( GL_TEXTURE_2D );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    GLfloat aniso = 0.0f;
    glGetFloatv( 0x84FF /* GL_MAX_TEXTURE_MAX_ANISOTROPY */, &aniso );
    if ( aniso > 1.0f ) glTexParameterf( GL_TEXTURE_2D, 0x84FE /* GL_TEXTURE_MAX_ANISOTROPY */, std::min( aniso, 8.0f ) );
    glGetError(); // anisotropy is an extension in 3.3

    // world x = field x, world z = field y; image rows go down = +y
    const float x0 = -W / 2, x1 = W / 2, z0 = -H / 2, z1 = H / 2;
    // counterclockwise seen from above
    const float v[] = { x0, 0, z0, 0, 0,  x1, 0, z1, 1, 1,  x1, 0, z0, 1, 0,
                        x0, 0, z0, 0, 0,  x0, 0, z1, 0, 1,  x1, 0, z1, 1, 1 };
    glGenVertexArrays( 1, &M_ground_vao );
    glBindVertexArray( M_ground_vao );
    glGenBuffers( 1, &M_ground_vbo );
    glBindBuffer( GL_ARRAY_BUFFER, M_ground_vbo );
    glBufferData( GL_ARRAY_BUFFER, sizeof( v ), v, GL_STATIC_DRAW );
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof( float ), nullptr );
    glEnableVertexAttribArray( 1 );
    glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof( float ), reinterpret_cast< void * >( 3 * sizeof( float ) ) );
    glBindVertexArray( 0 );
}

/*-------------------------------------------------------------------*/
void
SceneGL::createShadowMap()
{
    glGenTextures( 1, &M_shadow_tex );
    glBindTexture( GL_TEXTURE_2D, M_shadow_tex );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, options_.shadow_size, options_.shadow_size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL );
    glGenFramebuffers( 1, &M_shadow_fbo );
    glBindFramebuffer( GL_FRAMEBUFFER, M_shadow_fbo );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, M_shadow_tex, 0 );
    glDrawBuffer( GL_NONE );
    glReadBuffer( GL_NONE );
    if ( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
    {
        std::fprintf( stderr, "the shadow framebuffer is not complete\n" );
    }
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
}

/*-------------------------------------------------------------------*/
void
SceneGL::updateInstances( double play_time,
                          QVector3D * ball )
{
    const double t = options_.start_frame + play_time;
    float bx, by;
    M_log.ballAt( t, &bx, &by );
    *ball = QVector3D( bx, 0.0f, by );

    static const std::vector< int > clips = { M_rig.clipIndex( "Idle_Loop" ), M_rig.clipIndex( "Walk_Loop" ),
                                              M_rig.clipIndex( "Jog_Fwd_Loop" ), M_rig.clipIndex( "Sprint_Loop" ) };
    std::vector< float > times( 4 ), weights( 4 );
    const float scale = FIGURE_HEIGHT / M_rig.height_;
    const int joints = M_rig.jointCount();

    for ( int i = 0; i < PROTO_PLAYERS; ++i )
    {
        const ProtoFigure fig = M_log.figureAt( t, i );
        QMatrix4x4 model;
        model.translate( fig.x, 0.0f, fig.y );
        model.rotate( 90.0f - fig.body, 0.0f, 1.0f, 0.0f );

        if ( options_.figures )
        {
            times.assign( fig.times, fig.times + 4 );
            weights.assign( fig.weights, fig.weights + 4 );
            model.scale( scale );
            M_rig.pose( clips, times, weights, model, &M_joint_data[i * joints * 16] );
        }
        else
        {
            model.scale( 0.3f, 1.8f, 0.3f );
            std::copy_n( model.constData(), 16, &M_cyl_data[i * INSTANCE_FLOATS] );
            std::copy_n( &M_colors[i * 3], 3, &M_cyl_data[i * INSTANCE_FLOATS + 16] );
            M_cyl_data[i * INSTANCE_FLOATS + 19] = 1.0f;
        }
    }

    QMatrix4x4 m;
    m.translate( ball->x(), 0.2f, ball->z() );
    m.scale( 0.2f );
    std::copy_n( m.constData(), 16, M_sphere_data.data() );
    M_sphere_data[16] = M_sphere_data[17] = M_sphere_data[18] = M_sphere_data[19] = 1.0f;

    if ( options_.figures )
    {
        glBindTexture( GL_TEXTURE_2D, M_joint_tex );
        glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, joints * 4, PROTO_PLAYERS, GL_RGBA, GL_FLOAT, M_joint_data.data() );
    }
    else
    {
        glBindBuffer( GL_ARRAY_BUFFER, M_cyl_inst );
        glBufferData( GL_ARRAY_BUFFER, M_cyl_data.size() * sizeof( float ), M_cyl_data.data(), GL_STREAM_DRAW );
    }
    glBindBuffer( GL_ARRAY_BUFFER, M_sphere_inst );
    glBufferData( GL_ARRAY_BUFFER, M_sphere_data.size() * sizeof( float ), M_sphere_data.data(), GL_STREAM_DRAW );
}

/*-------------------------------------------------------------------*/
void
SceneGL::drawScene( bool depth_only,
                   const QMatrix4x4 & view_proj,
                   const QMatrix4x4 & light_view_proj )
{
    const auto common = [&]( QOpenGLShaderProgram * p )
        {
            p->bind();
            p->setUniformValue( "uViewProj", view_proj );
            p->setUniformValue( "uLightViewProj", light_view_proj );
            if ( ! depth_only )
            {
                p->setUniformValue( "uShadow", 1 );
                p->setUniformValue( "uShadows", options_.shadows ? 1 : 0 );
                p->setUniformValue( "uShadowTexel", 1.0f / options_.shadow_size );
                p->setUniformValue( "uSun", SUN );
            }
        };

    if ( ! depth_only )
    {
        common( M_ground.get() );
        M_ground->setUniformValue( "uTexture", 0 );
        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, M_ground_tex );
        glBindVertexArray( M_ground_vao );
        glDrawArrays( GL_TRIANGLES, 0, 6 );
    }

    if ( options_.figures )
    {
        QOpenGLShaderProgram * p = ( depth_only ? M_skinned_depth.get() : M_skinned.get() );
        common( p );
        p->setUniformValue( "uJoints", 2 );
        if ( ! depth_only )
        {
            p->setUniformValueArray( "uColors", M_colors, PROTO_PLAYERS, 3 );
        }
        glActiveTexture( GL_TEXTURE2 );
        glBindTexture( GL_TEXTURE_2D, M_joint_tex );
        glBindVertexArray( M_rig_vao );
        for ( const RigPrimitive & prim : M_rig.primitives_ )
        {
            p->setUniformValue( "uJersey", prim.material == "M_Main" ? 1 : 0 );
            glDrawElementsInstanced( GL_TRIANGLES, prim.index_count, GL_UNSIGNED_INT,
                                     reinterpret_cast< void * >( prim.first_index * sizeof( std::uint32_t ) ),
                                     PROTO_PLAYERS );
        }
    }

    QOpenGLShaderProgram * ip = ( depth_only ? M_instanced_depth.get() : M_instanced.get() );
    common( ip );
    if ( ! options_.figures )
    {
        glBindVertexArray( M_cyl_vao );
        glDrawElementsInstanced( GL_TRIANGLES, M_cyl_count, GL_UNSIGNED_INT, nullptr, PROTO_PLAYERS );
    }
    glBindVertexArray( M_sphere_vao );
    glDrawElementsInstanced( GL_TRIANGLES, M_sphere_count, GL_UNSIGNED_INT, nullptr, 1 );
    glBindVertexArray( 0 );
}

/*-------------------------------------------------------------------*/
double
SceneGL::render( GLuint fbo,
                 int fb_w,
                 int fb_h,
                 double play_time )
{
    QElapsedTimer cpu;
    cpu.start();

    glBeginQuery( GL_TIME_ELAPSED, M_queries[M_query_index] );

    QVector3D ball;
    {
        QElapsedTimer pose;
        pose.start();
        updateInstances( play_time, &ball );
        M_pose_ms = pose.nsecsElapsed() * 1e-6;
    }

    // the camera orbits the ball, kept 10 m inside the pitch
    const QVector3D target( std::clamp( ball.x(), -42.5f, 42.5f ), 0.0f, std::clamp( ball.z(), -24.0f, 24.0f ) );
    const float el = elevation_ * float( M_PI ) / 180.0f, az = azimuth_ * float( M_PI ) / 180.0f;
    const QVector3D eye = target + distance_ * QVector3D( std::cos( el ) * std::cos( az ), std::sin( el ), std::cos( el ) * std::sin( az ) );
    QMatrix4x4 view;
    view.lookAt( eye, target, QVector3D( 0, 1, 0 ) );
    QMatrix4x4 proj;
    proj.perspective( 35.0f, float( fb_w ) / std::max( 1, fb_h ), 0.5f, 2000.0f );
    const QMatrix4x4 view_proj = proj * view;
    static const QMatrix4x4 light = light_view_proj();

    glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );

    if ( options_.shadows )
    {
        glBindFramebuffer( GL_FRAMEBUFFER, M_shadow_fbo );
        glViewport( 0, 0, options_.shadow_size, options_.shadow_size );
        glClear( GL_DEPTH_BUFFER_BIT );
        glDisable( GL_CULL_FACE );
        drawScene( true, light, light );
        glEnable( GL_CULL_FACE );
    }

    glBindFramebuffer( GL_FRAMEBUFFER, fbo );
    glViewport( 0, 0, fb_w, fb_h );
    glClearColor( 0.55f, 0.72f, 0.88f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_2D, M_shadow_tex );
    drawScene( false, view_proj, light );

    glEndQuery( GL_TIME_ELAPSED );
    M_query_pending[M_query_index] = true;
    M_query_index = 1 - M_query_index;
    if ( M_query_pending[M_query_index] )
    {
        GLint available = 0;
        glGetQueryObjectiv( M_queries[M_query_index], GL_QUERY_RESULT_AVAILABLE, &available );
        if ( available )
        {
            GLuint64 ns = 0;
            glGetQueryObjectui64v( M_queries[M_query_index], GL_QUERY_RESULT, &ns );
            M_query_pending[M_query_index] = false;
            M_gpu_ms = ns * 1e-6;
        }
    }

    glDisable( GL_CULL_FACE );
    glDisable( GL_DEPTH_TEST );
    glBindVertexArray( 0 );
    glActiveTexture( GL_TEXTURE0 );
    return cpu.nsecsElapsed() * 1e-6;
}

/*-------------------------------------------------------------------*/
double
SceneGL::takeGpuMs()
{
    const double ms = M_gpu_ms;
    M_gpu_ms = -1.0;
    return ms;
}
