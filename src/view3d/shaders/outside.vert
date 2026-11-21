// The ground beyond the ground image (Scenery3D): a plain plane, melting
// into the horizon in the haze far away.
// See ground.vert for how a vertex shader works.
//
// ShaderProgram adds "#version 330 core" and the A_* locations
// (gl/attributes.h) before these lines.

// of the vertex: the plane (Meshes3D::ground())
layout( location = A_POSITION ) in vec3 aPos;
// of the instance: where it is, and its color
layout( location = A_MODEL ) in mat4 iModel;
layout( location = A_COLOR ) in vec4 iColor;

uniform mat4 uViewProj;

out vec3 vWorld;
out vec4 vColor;

void main()
{
    vec4 world = iModel * vec4( aPos, 1.0 );
    vWorld = world.xyz;
    vColor = iColor;
    gl_Position = uViewProj * world;
}
