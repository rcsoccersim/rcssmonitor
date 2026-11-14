// The plane of the sun's shadows, just over the ground (Sun3D): darkened
// where something nearer to the sun is over it in the shadow map.
// See ground.vert for how a vertex shader works.
//
// ShaderProgram adds "#version 330 core" and the A_* locations
// (gl/attributes.h) before these lines.

// of the vertex: the plane (Meshes3D::ground())
layout( location = A_POSITION ) in vec3 aPos;
// of the instance: where the plane is
layout( location = A_MODEL ) in mat4 iModel;

uniform mat4 uViewProj;      // the camera
uniform mat4 uSunViewProj;   // the sun's camera (Sun3D::shadowViewProj())

out vec3 vSun; // where this point is in the shadow map: 0 to 1 across and down, and its depth

void main()
{
    vec4 world = iModel * vec4( aPos, 1.0 );
    // the sun's camera gives -1 to 1 (orthographic: no division needed); the map is read 0 to 1
    vSun = ( uSunViewProj * world ).xyz * 0.5 + 0.5;
    gl_Position = uViewProj * world;
}
