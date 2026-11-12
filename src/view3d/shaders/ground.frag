// The ground: the image of the 2D view (GroundTexture), unlit.
//
// A fragment shader runs once for every pixel that a triangle covers, with
// the "out" values of the vertex shader interpolated between the corners of
// the triangle, and writes the pixel's color (fragColor).

in vec2 vUV; // where in the image this pixel is

uniform sampler2D uTexture; // the image (Texture2D), on the texture unit set by Scene3D::render()

out vec4 fragColor; // red, green, blue, alpha, 0 to 1

void main()
{
    fragColor = vec4( texture( uTexture, vUV ).rgb, 1.0 );
}
