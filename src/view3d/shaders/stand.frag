// The stands (stand.vert): the image, lit as lit.frag lights the solids
// (all around, from the sky above, and by the sun).
//
// ShaderProgram adds "#version 330 core" and the A_* locations
// (gl/attributes.h) before these lines.

in vec3 vNormal;
in vec2 vUV;

uniform sampler2D uTexture; // the stand's image (Stands3D::image())
uniform vec3 uSun;          // toward the sun, of length 1

out vec4 fragColor;

void main()
{
    vec3 n = normalize( vNormal );
    float ambient = 0.42;
    float sky = 0.22 * ( 0.5 + 0.5 * n.y );
    float sun = 0.5 * max( dot( n, uSun ), 0.0 );
    vec3 c = texture( uTexture, vUV ).rgb;
    fragColor = vec4( min( c * ( ambient + sky + sun ), vec3( 1.0 ) ), 1.0 );
}
