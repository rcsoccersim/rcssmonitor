// The simple shadows (blob.vert): as dark as the instance's alpha out to
// 0.35 of the way to the edge, then fading to nothing at the edge, so
// that the ellipse has no hard outline.
//
// ShaderProgram adds "#version 330 core" and the A_* locations
// (gl/attributes.h) before these lines.

in vec2 vDisc;
in vec4 vColor;

out vec4 fragColor;

void main()
{
    float r = length( vDisc ); // interpolated across a triangle of the disc: exact
    fragColor = vec4( vColor.rgb, vColor.a * ( 1.0 - smoothstep( 0.35, 1.0, r ) ) );
}
