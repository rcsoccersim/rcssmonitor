// The panels around the pitch: the image on the front, the back plain.
//
// gl_FrontFacing tells whether the triangle is seen from its front (+z of
// the quad, its corners counterclockwise on the screen): the render state
// draws both faces, and the back takes the instance's color.

in vec2 vUV;
in vec4 vColor;

uniform sampler2D uTexture; // the image of the panel's end (Texture2D)

out vec4 fragColor;

void main()
{
    fragColor = gl_FrontFacing ? vec4( texture( uTexture, vUV ).rgb, 1.0 ) : vec4( vColor.rgb, 1.0 );
}
