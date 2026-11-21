// The flat colors, translucent by the alpha: RenderState::translucent()
// mixes them with what is behind.
// See ground.frag for how a fragment shader works.

in vec4 vColor;

out vec4 fragColor;

void main()
{
    fragColor = vColor;
}
