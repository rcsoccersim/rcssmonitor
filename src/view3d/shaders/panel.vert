// The panels around the pitch (the boards and the banners, Stadium3D): an
// image on the front of the unit quad, unlit, and the back plain.
//
// ShaderProgram adds "#version 330 core" and the A_* locations
// (gl/attributes.h) before these lines.

// of the vertex (Meshes3D::quad(): 1 x 1 on the xy plane, facing +z)
layout( location = A_POSITION ) in vec3 aPos;
layout( location = A_UV ) in vec2 aUV;        // (0, 0) at the bottom left to (1, 1) at the top right
// of the instance (Instance3D): where the panel is, and the color of its back
layout( location = A_MODEL ) in mat4 iModel;
layout( location = A_COLOR ) in vec4 iColor;

uniform mat4 uViewProj;
// the image once in every this many meters across (the boards), or the
// width of the panel (the banners: once)
uniform float uTile;

out vec2 vUV;
out vec4 vColor;

void main()
{
    // across: as many images as the panel is wide (the length of the
    // model's x axis) over the tile; up: the image's top row (0) at the top
    float width = length( iModel[0].xyz );
    vUV = vec2( aUV.x * width / uTile, 1.0 - aUV.y );
    vColor = iColor;
    gl_Position = uViewProj * iModel * vec4( aPos, 1.0 );
}
