// The sky (Scenery3D): a dome around the camera, drawn first, behind all.
// See ground.vert for how a vertex shader works.
//
// ShaderProgram adds "#version 330 core" and the A_* locations
// (gl/attributes.h) before these lines.

// of the vertex: the unit sphere (Meshes3D::sphere())
layout( location = A_POSITION ) in vec3 aPos;
// of the instance: the dome, at the camera and SKY_RADIUS large
layout( location = A_MODEL ) in mat4 iModel;

uniform mat4 uViewProj;

out vec3 vDirection; // from the camera

void main()
{
    vDirection = aPos;
    gl_Position = uViewProj * iModel * vec4( aPos, 1.0 );
}
