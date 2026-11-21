// The solids (the players, the ball, the goal posts), lit by the sun.
// See ground.vert for how a vertex shader works.
//
// ShaderProgram adds "#version 330 core" and the A_* locations
// (gl/attributes.h) before these lines.

// of the vertex: a shape of unit size (Meshes3D)
layout( location = A_POSITION ) in vec3 aPos;
layout( location = A_NORMAL ) in vec3 aNormal; // the direction the surface faces
// of the instance: where this copy of the shape is, and its color (SceneBuilder)
layout( location = A_MODEL ) in mat4 iModel;
layout( location = A_COLOR ) in vec4 iColor;

// the camera: the world to the screen
uniform mat4 uViewProj;

out vec3 vNormal; // in the world
out vec4 vColor;

void main()
{
    // Turn the normal with the shape. A scale that is not the same along
    // every axis (a thin cylinder) would tilt the normal off the surface if
    // it were multiplied by iModel itself; the "normal matrix", the inverse
    // transpose, keeps it perpendicular.
    vNormal = transpose( inverse( mat3( iModel ) ) ) * aNormal;
    vColor = iColor;
    gl_Position = uViewProj * iModel * vec4( aPos, 1.0 );
}
