// The solids that cast the sun's shadows (the players' bodies and heads,
// the goal posts and crossbars), drawn as the sun sees them into the shadow
// map (Sun3D, gl/shadow_map): only where they are, for their depths.
// See ground.vert for how a vertex shader works.
//
// ShaderProgram adds "#version 330 core" and the A_* locations
// (gl/attributes.h) before these lines.

// of the vertex: a shape of unit size (Meshes3D)
layout( location = A_POSITION ) in vec3 aPos;
// of the instance: where this copy of the shape is (SceneBuilder)
layout( location = A_MODEL ) in mat4 iModel;

uniform mat4 uViewProj; // the sun's camera (Sun3D::shadowViewProj())

void main()
{
    gl_Position = uViewProj * iModel * vec4( aPos, 1.0 );
}
