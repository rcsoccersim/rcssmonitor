// The ball, lit as the solids (lit.vert), with the direction on the ball
// itself (before it is turned) for its pattern (ball.frag).
//
// ShaderProgram adds "#version 330 core" and the A_* locations
// (gl/attributes.h) before these lines.

// of the vertex: a sphere of radius 1 (Meshes3D)
layout( location = A_POSITION ) in vec3 aPos;
layout( location = A_NORMAL ) in vec3 aNormal;
// of the instance: where the ball is, how it is turned, and its color (SceneBuilder)
layout( location = A_MODEL ) in mat4 iModel;
layout( location = A_COLOR ) in vec4 iColor;

uniform mat4 uViewProj;

out vec3 vNormal; // in the world
out vec3 vDirection; // on the ball itself: turns with it
out vec4 vColor;

void main()
{
    vNormal = transpose( inverse( mat3( iModel ) ) ) * aNormal;
    vDirection = aPos;
    vColor = iColor;
    gl_Position = uViewProj * iModel * vec4( aPos, 1.0 );
}
