// The ground: the image of the 2D view (GroundTexture), unlit.
//
// A fragment shader runs once for every pixel that a triangle covers, with
// the "out" values of the vertex shader interpolated between the corners of
// the triangle, and writes the pixel's color (fragColor).

in vec2 vUV; // where in the image this pixel is
in vec3 vWorld; // and in the world

uniform sampler2D uTexture; // the image (Texture2D), on the texture unit set by Scene3D::render()

// the realistic grass (Grass3D, whose functions compute the same): the
// colors of the image (with the patches and the wear painted in) times the
// mowed stripes by the way the camera looks, and the blades near the camera
uniform int uRealistic;      // 0: the image as it is
uniform sampler2D uNoise;    // the noise (Grass3D::noiseImage()), tiling both ways
uniform vec3 uCamera;        // where the camera is
uniform float uStripeWidth;  // FieldPainter::GRASS_STRIPE_WIDTH
uniform float uStripeContrast;
uniform float uStripeFixed;
uniform float uDetailTile;
uniform float uDetailAmount;
uniform float uBladesNear;   // the blades in full nearer than this (m)
uniform float uBladesFar;    // and none farther: no texture read there

out vec4 fragColor; // red, green, blue, alpha, 0 to 1

void main()
{
    vec3 c = texture( uTexture, vUV ).rgb;
    if ( uRealistic != 0 )
    {
        vec2 p = vWorld.xz;

        // the stripe and the way it was mowed: the even ones toward -z,
        // the odd ones back (mod is never negative in GLSL)
        float light = ( mod( floor( p.x / uStripeWidth ), 2.0 ) == 0.0 ? 1.0 : -1.0 );
        vec3 view = vec3( p.x, 0.0, p.y ) - uCamera;
        float len = length( view );
        float along = ( len > 1.0e-6 ? dot( view.xz / len, vec2( 0.0, -light ) ) : 0.0 );
        c *= 1.0 + uStripeContrast * ( uStripeFixed * light + ( 1.0 - uStripeFixed ) * along );

        // the blades, near the camera only (far away they would average out)
        if ( len < uBladesFar )
        {
            float blades = texture( uNoise, p / uDetailTile ).r;
            c *= 1.0 + uDetailAmount * ( blades - 0.5 ) * ( 1.0 - smoothstep( uBladesNear, uBladesFar, len ) );
        }
    }
    fragColor = vec4( min( c, vec3( 1.0 ) ), 1.0 );
}
