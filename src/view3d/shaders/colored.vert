// The vertices with their own colors (VertexBatch): the lines and dots of
// the overlays on the ground, and the view areas.
// See ground.vert for how a vertex shader works.
//
// ShaderProgram adds "#version 330 core" and the A_* locations
// (gl/attributes.h) before these lines.

layout( location = A_POSITION ) in vec3 aPos;   // in the world
layout( location = A_COLOR ) in vec4 aColor;    // red, green, blue, opacity
layout( location = A_DISTANCE ) in float aDistance; // along a dotted line (m)

uniform mat4 uViewProj;
uniform float uPointSize; // of the dots (pixels), when drawn as points

out vec4 vColor;
out float vDistance;

void main()
{
    vColor = aColor;
    vDistance = aDistance;
    gl_PointSize = uPointSize;
    gl_Position = uViewProj * vec4( aPos, 1.0 );
}
