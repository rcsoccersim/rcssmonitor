// The stands (Stands3D): their faces with the stand's image, lit by the sun
// as the solids (lit.frag).
// See ground.vert for how a vertex shader works.
//
// ShaderProgram adds "#version 330 core" and the A_* locations
// (gl/attributes.h) before these lines.

// of the vertex: in the world already (the stand's mesh), its normal and where in the image
layout( location = A_POSITION ) in vec3 aPos;
layout( location = A_NORMAL ) in vec3 aNormal;
layout( location = A_UV ) in vec2 aUV;
// of the instance: the identity (one stand)
layout( location = A_MODEL ) in mat4 iModel;

uniform mat4 uViewProj;

out vec3 vNormal;
out vec2 vUV;

void main()
{
    vNormal = mat3( iModel ) * aNormal;
    vUV = aUV;
    gl_Position = uViewProj * iModel * vec4( aPos, 1.0 );
}
