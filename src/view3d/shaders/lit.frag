// The solids, lit by the sun and by the sky.
// See ground.frag for how a fragment shader works.

in vec3 vNormal; // interpolated across the triangle: no longer of length 1
in vec4 vColor;

uniform vec3 uSun; // toward the sun, of length 1

out vec4 fragColor;

void main()
{
    vec3 n = normalize( vNormal );

    // The brightness, from three lights (tuned by looking; up to about 1.1):
    float ambient = 0.42;                    // everywhere, so that no side is black
    float sky = 0.22 * ( 0.5 + 0.5 * n.y );  // from above: brighter facing up
    float sun = 0.5 * max( dot( n, uSun ), 0.0 ); // Lambert: brightest facing the sun, none facing away

    fragColor = vec4( min( vColor.rgb * ( ambient + sky + sun ), vec3( 1.0 ) ), vColor.a );
}
