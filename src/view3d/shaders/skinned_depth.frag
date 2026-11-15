// The depths of the figures' parts with a texture or a transparency in the
// shadow map (with skinned.vert; the plain parts with depth.frag): the GPU
// writes the depth of every pixel by itself, but not where a part of a
// texture is transparent (a mask: hair cards, eyelashes), which casts no
// shadow there, as skinned.frag leaves it out (SkinnedMesh sets the
// uniforms). ShaderProgram adds "#define TEXTURED" (skinned.vert passes
// vUV).
//
// ShaderProgram adds "#version 330 core" and the A_* locations
// (gl/attributes.h) before these lines.

in vec2 vUV;
in vec4 vColor; // its alpha: the material's own, or 1

uniform int uTextured;      // the material has a texture
uniform sampler2D uTexture; // its texture
uniform float uAlphaCutoff; // not drawn where more transparent than this (< 0: opaque)

void main()
{
    if ( uAlphaCutoff >= 0.0 )
    {
        float alpha = vColor.a * ( uTextured != 0 ? texture( uTexture, vUV ).a : 1.0 );
        if ( alpha < uAlphaCutoff ) discard;
    }
}
