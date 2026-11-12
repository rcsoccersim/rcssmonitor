// The ground: the image of the 2D view (GroundTexture) on a rectangle, unlit.
//
// A vertex shader runs once for every vertex of the triangles drawn. It
// reads the vertex ("in", from the buffers of InstancedMesh), writes where
// the vertex is on the screen (gl_Position), and passes values ("out") to
// the fragment shader (ground.frag).
//
// ShaderProgram adds "#version 330 core" and the A_* locations
// (gl/attributes.h) before these lines.

// of the vertex (Meshes3D::ground())
layout( location = A_POSITION ) in vec3 aPos; // in meters, the world's (the ground's model matrix is the identity)
layout( location = A_UV ) in vec2 aUV;        // where in the image: (0, 0) to (1, 1)
// of the instance (Instance3D)
layout( location = A_MODEL ) in mat4 iModel;

// the camera: the world to the screen (CameraPose::projection() * view()), set by Scene3D::render()
uniform mat4 uViewProj;

out vec2 vUV;

void main()
{
    vUV = aUV;
    // the shape's space -> the world (iModel) -> the screen (uViewProj)
    gl_Position = uViewProj * iModel * vec4( aPos, 1.0 );
}
