// The ring on the ground around the ball (BallAids3D): a square of 2 x 2 on
// the ground (Meshes3D::ground()), as large as the ring, which ring.frag
// draws the line and the fill on.
// See ground.vert for how a vertex shader works.
//
// ShaderProgram adds "#version 330 core" and the A_* locations
// (gl/attributes.h) before these lines.

// of the vertex: the square from -1 to 1 on the xz plane
layout( location = A_POSITION ) in vec3 aPos;
// of the instance: around the ball, scaled by the ring's radius; the ball's color
layout( location = A_MODEL ) in mat4 iModel;
layout( location = A_COLOR ) in vec4 iColor;

uniform mat4 uViewProj;

out vec2 vAt; // where on the square: its length is how far out, of the radius
out vec4 vColor;

void main()
{
    vAt = aPos.xz;
    vColor = iColor;
    gl_Position = uViewProj * iModel * vec4( aPos, 1.0 );
}
