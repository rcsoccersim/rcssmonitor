// The ball, lit as the solids (lit.frag), with the pattern of a football:
// twelve dark pentagons at the corners of an icosahedron (Ball3D), found
// from the direction on the ball at each pixel, so that their edges stay
// sharp however near the camera is. Ball3D::patternAt() is the same on the
// CPU.

in vec3 vNormal;
in vec3 vDirection;
in vec4 vColor;

uniform vec3 uSun; // toward the sun, of length 1
uniform int uPatterned; // 1: the pentagons (the 3D tab's Ball Pattern), 0: plain
uniform vec3 uCenters[12]; // the pentagons' centers (Ball3D::pentagons())
uniform vec3 uToward[12]; // for each, the direction of a corner
uniform float uPentagon; // the corners from the centers (radians)
uniform float uShade; // how dark a pentagon is

out vec4 fragColor;

// PENTAGON_SHADE in a pentagon, 1 outside, the edge smoothed over a pixel
float pattern( vec3 n )
{
    // the nearest pentagon
    int best = 0;
    float nearest = -2.0;
    for ( int i = 0; i < 12; ++i )
    {
        float d = dot( n, uCenters[i] );
        if ( d > nearest )
        {
            nearest = d;
            best = i;
        }
    }
    vec3 v = uCenters[best];
    vec3 e1 = normalize( uToward[best] - v * dot( uToward[best], v ) );
    vec3 e2 = cross( v, e1 );
    // around the center from a corner, in sectors of 72 degrees: the
    // distance toward the nearest edge, whose normal is 36 degrees off the
    // corners
    float sector = 1.2566371;
    float a = mod( atan( dot( n, e2 ), dot( n, e1 ) ), sector ) - 0.5 * sector;
    float r = acos( clamp( nearest, -1.0, 1.0 ) ) * cos( a );
    float edge = uPentagon * 0.8090170; // cos(36 degrees)
    float w = max( fwidth( r ), 1.0e-4 ); // a pixel, so that the edge is smooth
    return mix( uShade, 1.0, smoothstep( edge - w, edge + w, r ) );
}

void main()
{
    vec3 n = normalize( vNormal );

    // as lit.frag
    float ambient = 0.42;
    float sky = 0.22 * ( 0.5 + 0.5 * n.y );
    float sun = 0.5 * max( dot( n, uSun ), 0.0 );

    vec3 color = vColor.rgb;
    if ( uPatterned != 0 )
    {
        color *= pattern( normalize( vDirection ) );
    }
    fragColor = vec4( min( color * ( ambient + sky + sun ), vec3( 1.0 ) ), vColor.a );
}
