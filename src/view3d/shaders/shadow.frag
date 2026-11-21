// The plane of the sun's shadows (shadow.vert): black, as opaque as the
// part of the point that is in the shadow, times the darkness of a shadow.
//
// ShaderProgram adds "#version 330 core" and the A_* locations
// (gl/attributes.h) before these lines.

in vec3 vSun;

uniform sampler2DShadow uShadowMap; // the depths (gl/shadow_map): read by comparing
uniform float uTexel;               // one pixel of the map, 0 to 1
uniform float uOpacity;             // a full shadow (Sun3D::SHADOW_OPACITY)

out vec4 fragColor;

// how far a point of the plane may be beyond the depth in the map and
// still be lit, against the map's rounding (about 7 cm over the sun's
// 150 m of depth). The plane casts nothing, so it need not be more.
const float BIAS = 0.0005;

// how much of the point is lit through the map at an offset (pixels of the map)
float litAt( vec2 offset )
{
    return texture( uShadowMap, vec3( vSun.xy + offset * uTexel, vSun.z - BIAS ) );
}

void main()
{
    // lit: 1, in the shadow: 0. Each read compares the 4 pixels of the map
    // around the point; 5 x 5 reads a pixel apart soften the edge over
    // about 5 pixels (about 15 cm). Most of the ground is far from any
    // shadow (or deep in one): 5 reads first, the middle and the corners,
    // and the other 20 only where they differ, near an edge. Without that,
    // the plane took about 7 ms of an integrated GPU at 2480 x 1395 pixels.
    float probe = litAt( vec2( 0.0, 0.0 ) )
        + litAt( vec2( -2.0, -2.0 ) ) + litAt( vec2( 2.0, -2.0 ) )
        + litAt( vec2( -2.0, 2.0 ) ) + litAt( vec2( 2.0, 2.0 ) );
    if ( probe > 4.999 )
    {
        discard; // all lit: nothing to darken
    }
    float lit = 0.0;
    if ( probe > 0.001 )
    {
        for ( int y = -2; y <= 2; ++y )
        {
            for ( int x = -2; x <= 2; ++x )
            {
                lit += litAt( vec2( x, y ) );
            }
        }
        lit /= 25.0;
    }
    fragColor = vec4( 0.0, 0.0, 0.0, uOpacity * ( 1.0 - lit ) );
}
