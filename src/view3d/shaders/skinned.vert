// The human figures (SkinnedMesh): each vertex moves with up to 4 joints
// of its figure's skeleton, by their weights. The skin matrices of the
// joints are in a texture, a row a figure (gl_InstanceID), 4 texels (the
// columns) a joint. Lit and painted by skinned.frag.

layout( location = A_POSITION ) in vec3 aPos;
layout( location = A_NORMAL ) in vec3 aNormal;
layout( location = A_JOINTS ) in uvec4 aJoints;
layout( location = A_WEIGHTS ) in vec4 aWeights;
layout( location = A_REST ) in vec3 aRest;  // where the vertex is at rest, for the uniform
layout( location = A_PART ) in float aPart; // the part of the body, for the uniform
layout( location = A_COLOR ) in vec4 iColor; // the figure's color

uniform mat4 uViewProj;
uniform sampler2D uJoints;   // the skin matrices
uniform int uPaint;          // 0: this part in uBaseColor; 1: in the figure's color; 2: in the uniform of it
uniform vec4 uBaseColor;     // the color of the material

out vec3 vNormal; // in the world
out vec4 vColor;  // uBaseColor or the figure's color
out vec3 vRest;
flat out float vPart; // not interpolated: the part of the triangle's first vertex

mat4 jointMatrix( uint joint )
{
    int x = int( joint ) * 4;
    int y = gl_InstanceID;
    return mat4( texelFetch( uJoints, ivec2( x, y ), 0 ),
                 texelFetch( uJoints, ivec2( x + 1, y ), 0 ),
                 texelFetch( uJoints, ivec2( x + 2, y ), 0 ),
                 texelFetch( uJoints, ivec2( x + 3, y ), 0 ) );
}

void main()
{
    mat4 skin = aWeights.x * jointMatrix( aJoints.x )
              + aWeights.y * jointMatrix( aJoints.y )
              + aWeights.z * jointMatrix( aJoints.z )
              + aWeights.w * jointMatrix( aJoints.w );
    // the normal by the inverse transpose, as lit.vert (the skin may scale)
    vNormal = transpose( inverse( mat3( skin ) ) ) * aNormal;
    vColor = ( uPaint != 0 ? iColor : uBaseColor );
    vRest = aRest;
    vPart = aPart;
    gl_Position = uViewProj * skin * vec4( aPos, 1.0 );
}
