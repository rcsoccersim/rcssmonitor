// The billboards: the texture, tinted by the color (white stays the color,
// black stays black), translucent by its alpha.
// See ground.frag for how a fragment shader works.

in vec2 vUV;
in vec4 vColor;

uniform sampler2D uTexture; // the labels (LabelAtlas)

out vec4 fragColor;

void main()
{
    vec4 texel = texture( uTexture, vUV );
    fragColor = texel * vColor;
    // nothing where the texture is empty, not even the depth
    if ( fragColor.a < 0.01 )
    {
        discard;
    }
}
