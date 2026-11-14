// The human figures: lit as the solids (lit.frag), and a body in a uniform
// painted by the part of the body and where the point is at rest
// (FigureUniform, figure_uniform.cpp, decides the same).

in vec3 vNormal;
in vec4 vColor;
in vec4 vSkin;  // the figure's skin
in vec4 vHair;  // the figure's hair, the hairstyles it wears in w
in vec4 vScalp; // under each hairstyle of the model
in vec3 vRest;
in vec2 vPart; // how much the arms' and the legs' bones move it

uniform vec3 uSun;          // toward the sun, of length 1
uniform int uPaint;         // 2: in the uniform
uniform vec4 uCuts[13];     // the planes where the clothes end, the left side, the chin and the neck (FigureUniform::Cuts)
uniform vec4 uShoeColor;
uniform float uShortsShade; // the shorts: the figure's color times this
uniform float uCollarSlope; // the collar's width grows by this much for its height
uniform vec4 uScalp;        // the hairstyle (bit, as a float) of each of vScalp (FigureUniform::Cuts::scalp)

out vec4 fragColor;

// how far the point is beyond a plane
float beyond( int plane )
{
    return dot( uCuts[plane].xyz, vRest ) + uCuts[plane].w;
}

// FigureUniform::Cuts::inCollar(): the skin of the neck within the collar
// (above the collar, near the neck's middle line, wider higher up), and all
// above the chin
bool inCollar()
{
    vec3 axis = uCuts[8].xyz;
    vec3 d = vRest - uCuts[12].xyz;
    float h = beyond( 8 );
    return beyond( 11 ) > 0.0
        || ( h > 0.0 && length( d - axis * dot( d, axis ) ) < uCuts[12].w + uCollarSlope * h );
}

// FigureUniform::partAt(), regionAt() and colorOf(): the shirt and the
// socks in the figure's color
vec4 uniformColor()
{
    // an arm or a leg where its bones move it more than half (left or right
    // by the plane between the shoulders), else the torso with the head
    int side = ( beyond( 10 ) >= 0.0 ? 0 : 1 );
    int part = ( vPart.x > 0.5 ? 1 + side : vPart.y > 0.5 ? 3 + side : 0 );
    vec4 shorts = vec4( vColor.rgb * uShortsShade, vColor.a );
    if ( part == 1 || part == 2 )       // an arm: the sleeve, or the skin beyond it
    {
        return ( beyond( part - 1 ) > 0.0 ? vSkin : vColor );
    }
    if ( part == 3 || part == 4 )       // a leg: the shorts, the knee, the sock, the shoe
    {
        int s = part - 3;
        if ( beyond( 2 + s ) <= 0.0 ) return shorts;
        if ( beyond( 4 + s ) <= 0.0 ) return vSkin;
        if ( beyond( 6 + s ) <= 0.0 ) return vColor;
        return uShoeColor;
    }
    // the torso: under a hairstyle worn the hair (no scalp through its
    // gaps), the neck and the head above the collar, the shorts below the waist
    uint worn = uint( vHair.w + 0.5 );
    uvec4 bits = uvec4( uScalp + 0.5 );
    vec4 wears = vec4( ( worn & bits.x ) != 0u ? 1.0 : 0.0,
                       ( worn & bits.y ) != 0u ? 1.0 : 0.0,
                       ( worn & bits.z ) != 0u ? 1.0 : 0.0,
                       ( worn & bits.w ) != 0u ? 1.0 : 0.0 );
    if ( dot( vScalp, wears ) > 0.5 ) return vec4( vHair.rgb, 1.0 );
    if ( inCollar() ) return vSkin;
    if ( beyond( 9 ) > 0.0 ) return shorts;
    return vColor;
}

void main()
{
    vec3 n = normalize( vNormal );
    vec4 color = ( uPaint == 2 ? uniformColor() : vColor );

    // the brightness as lit.frag
    float ambient = 0.42;
    float sky = 0.22 * ( 0.5 + 0.5 * n.y );
    float sun = 0.5 * max( dot( n, uSun ), 0.0 );

    fragColor = vec4( min( color.rgb * ( ambient + sky + sun ), vec3( 1.0 ) ), color.a );
}
