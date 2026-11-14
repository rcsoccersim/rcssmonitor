// The simple shadows of the players (ShadowMode::Simple): the unit disc
// stretched into an ellipse on the ground away from the sun
// (Sun3D::blobShadow()), darkest in the middle.
// See ground.vert for how a vertex shader works.
//
// ShaderProgram adds "#version 330 core" and the A_* locations
// (gl/attributes.h) before these lines.

// of the vertex: the disc of radius 1 on the xz plane (Meshes3D::ring())
layout( location = A_POSITION ) in vec3 aPos;
// of the instance: the ellipse, and black with the darkness in the middle
layout( location = A_MODEL ) in mat4 iModel;
layout( location = A_COLOR ) in vec4 iColor;

uniform mat4 uViewProj;

out vec2 vDisc;  // where on the disc: its length is how far out (0 to 1)
out vec4 vColor;

void main()
{
    vDisc = aPos.xz;
    vColor = iColor;
    gl_Position = uViewProj * iModel * vec4( aPos, 1.0 );
}
