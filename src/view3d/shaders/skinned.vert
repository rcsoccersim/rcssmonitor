// The human figures (SkinnedMesh): each vertex moves with up to 4 joints
// of its figure's skeleton, by their weights. The skin matrices of the
// joints are in a texture, a row a figure (gl_InstanceID), 4 texels (the
// columns) a joint. Lit and painted by skinned.frag.

layout( location = A_POSITION ) in vec3 aPos;
layout( location = A_NORMAL ) in vec3 aNormal;
layout( location = A_JOINTS ) in uvec4 aJoints;
layout( location = A_WEIGHTS ) in vec4 aWeights;
layout( location = A_REST ) in vec3 aRest;  // where the vertex is at rest, for the uniform
layout( location = A_PART ) in vec2 aPart;  // how much the arms' and the legs' bones move it, for the uniform
layout( location = A_SCALP ) in vec4 aScalp; // under each hairstyle of the model or not
layout( location = A_COLOR ) in vec4 iColor; // the figure's color
layout( location = A_SKIN ) in vec4 iSkin;   // its skin
layout( location = A_HAIR ) in vec4 iHair;   // its hair, with the hairstyles it wears in w (bits)

uniform mat4 uViewProj;
uniform sampler2D uJoints;   // the skin matrices
uniform int uPaint;          // this part: 0 in uBaseColor; 1 in the figure's color; 2 in its uniform; 3 in its hair color
uniform int uShow;           // a hairstyle: its bit, drawn on the figures wearing it (0: on all)
uniform vec4 uBaseColor;     // the color of the material

out vec3 vNormal; // in the world
out vec4 vColor;  // uBaseColor, the figure's color or its hair's
out vec4 vSkin;   // the figure's skin
out vec4 vHair;   // the figure's hair, the hairstyles it wears in w
out vec4 vScalp;  // under each hairstyle (interpolated: a smooth hairline)
out vec3 vRest;
out vec2 vPart;   // interpolated: smooth between the parts

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
    vSkin = iSkin;
    vHair = iHair;
    vScalp = aScalp;
    vRest = aRest;
    vPart = aPart;
    // a hairstyle that the figure does not wear: out of sight (beyond the
    // far plane, as all of its triangles are, so that none is drawn)
    if ( uPaint == 3 && uShow != 0 && ( uint( iHair.w + 0.5 ) & uint( uShow ) ) == 0u )
    {
        vNormal = vec3( 0.0, 1.0, 0.0 );
        vColor = vec4( 0.0 );
        gl_Position = vec4( 0.0, 0.0, 2.0, 1.0 );
        return;
    }
    mat4 skin = aWeights.x * jointMatrix( aJoints.x )
              + aWeights.y * jointMatrix( aJoints.y )
              + aWeights.z * jointMatrix( aJoints.z )
              + aWeights.w * jointMatrix( aJoints.w );
    // the normal by the inverse transpose, as lit.vert (the skin may scale)
    vNormal = transpose( inverse( mat3( skin ) ) ) * aNormal;
    vColor = ( uPaint == 0 ? uBaseColor : uPaint == 3 ? vec4( iHair.rgb, 1.0 ) : iColor );
    gl_Position = uViewProj * skin * vec4( aPos, 1.0 );
}
