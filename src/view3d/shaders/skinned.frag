// The human figures: lit as the solids (lit.frag), and a body in a uniform
// painted by the part of the body and where the point is at rest
// (FigureUniform, figure_uniform.cpp, decides the same).

in vec3 vNormal;
in vec4 vColor;
in vec3 vRest;
flat in float vPart;

uniform vec3 uSun;          // toward the sun, of length 1
uniform int uPaint;         // 2: in the uniform
uniform vec4 uCuts[10];     // the planes where the clothes end (FigureUniform::Cuts)
uniform vec4 uSkinColor;
uniform vec4 uShoeColor;
uniform float uShortsShade; // the shorts: the figure's color times this

out vec4 fragColor;

// how far the point is beyond a plane
float beyond( int plane )
{
    return dot( uCuts[plane].xyz, vRest ) + uCuts[plane].w;
}

// FigureUniform::regionAt() and colorOf(): the shirt and the socks in the
// figure's color
vec4 uniformColor()
{
    int part = int( vPart + 0.5 );
    vec4 shorts = vec4( vColor.rgb * uShortsShade, vColor.a );
    if ( part == 5 ) return uSkinColor; // the head
    if ( part == 1 || part == 2 )       // an arm: the sleeve, or the skin beyond it
    {
        return ( beyond( part - 1 ) > 0.0 ? uSkinColor : vColor );
    }
    if ( part == 3 || part == 4 )       // a leg: the shorts, the knee, the sock, the shoe
    {
        int s = part - 3;
        if ( beyond( 2 + s ) <= 0.0 ) return shorts;
        if ( beyond( 4 + s ) <= 0.0 ) return uSkinColor;
        if ( beyond( 6 + s ) <= 0.0 ) return vColor;
        return uShoeColor;
    }
    // the torso: the neck above the collar, the shorts below the waist
    if ( beyond( 8 ) > 0.0 ) return uSkinColor;
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
