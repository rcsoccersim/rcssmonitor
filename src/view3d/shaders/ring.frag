// The ring on the ground around the ball (ring.vert): a line uWidth wide
// inside the radius uOuter, smoothed over a pixel, over a faint fill;
// nothing outside.

in vec2 vAt;
in vec4 vColor;

uniform float uOuter; // the ring's radius (m)
uniform float uWidth; // its line's width (m), at least a few pixels (BallAids3D::addRing())
uniform float uOpacity; // the line's
uniform float uFill; // inside the line

out vec4 fragColor;

void main()
{
    float d = length( vAt ) * uOuter; // from the center (m)
    float aa = max( fwidth( d ), 1.0e-4 ); // a pixel
    float inner = uOuter - uWidth;
    float in_ring = smoothstep( inner - aa, inner, d ) * ( 1.0 - smoothstep( uOuter - aa, uOuter, d ) );
    float inside = 1.0 - smoothstep( inner - aa, inner, d );
    float a = max( in_ring * uOpacity, inside * uFill );
    if ( a <= 0.0 ) discard;
    fragColor = vec4( vColor.rgb, a );
}
