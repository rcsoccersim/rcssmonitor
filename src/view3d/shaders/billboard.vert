// The billboards (the number labels and the cards): squares that face the camera.
// See ground.vert for how a vertex shader works.
//
// ShaderProgram adds "#version 330 core" and the A_* locations
// (gl/attributes.h) before these lines.

// of the vertex: the unit square (Meshes3D::quad()), -0.5 to 0.5 on x and y
layout( location = A_POSITION ) in vec3 aPos;
layout( location = A_UV ) in vec2 aUV; // (0, 0) at the bottom left, (1, 1) at the top right
// of the instance (Billboard3D)
layout( location = A_BILLBOARD_CENTER ) in vec3 iCenter;
layout( location = A_BILLBOARD_SIZE ) in vec4 iSize; // width, height, offset right, offset up (m)
layout( location = A_BILLBOARD_UV ) in vec4 iUV;     // left, top, right, bottom of its part of the texture
layout( location = A_COLOR ) in vec4 iColor;

uniform mat4 uViewProj;
uniform vec3 uRight; // the camera's right in the world: the rows of the view matrix
uniform vec3 uUp;    // the camera's up

out vec2 vUV;
out vec4 vColor;

void main()
{
    // spread the square along the camera's right and up: it faces the camera, upright on the screen
    vec2 local = aPos.xy * iSize.xy + iSize.zw;
    vec3 world = iCenter + uRight * local.x + uUp * local.y;
    // the image's rows go down: its top (iUV.y) at the top of the square
    vUV = vec2( mix( iUV.x, iUV.z, aUV.x ), mix( iUV.w, iUV.y, aUV.y ) );
    vColor = iColor;
    gl_Position = uViewProj * vec4( world, 1.0 );
}
