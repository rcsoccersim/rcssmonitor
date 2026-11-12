// The flat colors (the circles on the ground, the rings, the ball's shadow,
// the nets), translucent by the alpha, unlit.
// See ground.vert for how a vertex shader works.
//
// ShaderProgram adds "#version 330 core" and the A_* locations
// (gl/attributes.h) before these lines.

// of the vertex: a shape of unit size (Meshes3D)
layout( location = A_POSITION ) in vec3 aPos;
// of the instance: where this copy of the shape is, and its color with the opacity (SceneBuilder)
layout( location = A_MODEL ) in mat4 iModel;
layout( location = A_COLOR ) in vec4 iColor;

// the camera: the world to the screen
uniform mat4 uViewProj;

out vec4 vColor;

void main()
{
    vColor = iColor;
    gl_Position = uViewProj * iModel * vec4( aPos, 1.0 );
}
