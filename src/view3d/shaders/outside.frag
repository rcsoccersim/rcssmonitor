// The ground beyond (outside.vert): its color, and the haze over it by the
// distance from the camera, from uHazeFrom to uHazeTo (linearly, as
// Scenery3D::haze()), into the horizon's color. uHaze 0: no haze (no sky).
//
// ShaderProgram adds "#version 330 core" and the A_* locations
// (gl/attributes.h) before these lines.

in vec3 vWorld;
in vec4 vColor;

uniform vec3 uCamera;   // where the camera is
uniform float uHaze;    // 1 with the sky, 0 without
uniform vec3 uHorizon;  // the haze's color
uniform float uHazeFrom;
uniform float uHazeTo;

out vec4 fragColor;

void main()
{
    float f = uHaze * clamp( ( distance( vWorld, uCamera ) - uHazeFrom ) / ( uHazeTo - uHazeFrom ), 0.0, 1.0 );
    fragColor = vec4( mix( vColor.rgb, uHorizon, f ), 1.0 );
}
