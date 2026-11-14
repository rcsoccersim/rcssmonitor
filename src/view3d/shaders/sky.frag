// The sky (sky.vert): by how high the direction is, from the horizon to
// the zenith above (faster near the horizon), and below the horizon,
// quickly to the color of the ground beyond in the haze. As
// Scenery3D::skyColor().
//
// ShaderProgram adds "#version 330 core" and the A_* locations
// (gl/attributes.h) before these lines.

in vec3 vDirection;

uniform vec3 uZenith;
uniform vec3 uHorizon;
uniform vec3 uBelow;

out vec4 fragColor;

void main()
{
    float h = normalize( vDirection ).y;
    vec3 c = ( h >= 0.0 ? mix( uHorizon, uZenith, pow( h, 0.55 ) )
                        : mix( uHorizon, uBelow, min( 1.0, -h * 12.0 ) ) );
    fragColor = vec4( c, 1.0 );
}
