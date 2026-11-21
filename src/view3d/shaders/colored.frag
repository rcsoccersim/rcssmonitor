// The vertices with their own colors (VertexBatch), dotted if asked.
// See ground.frag for how a fragment shader works.

in vec4 vColor;
in float vDistance; // interpolated along the line

uniform float uDash; // > 0: dotted, this long dashes and gaps (m); 0: solid

out vec4 fragColor;

void main()
{
    // in a gap of a dotted line: nothing
    if ( uDash > 0.0 && mod( vDistance, 2.0 * uDash ) > uDash )
    {
        discard;
    }
    fragColor = vColor;
}
