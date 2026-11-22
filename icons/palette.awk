# palette.awk -- check and convert the colors of the icons with palette.txt
#
# Usage:
#   awk -v mode=palette -f palette.awk palette.txt
#       Check the format of palette.txt.
#   awk -v mode=check -f palette.awk palette.txt FILE.svg...
#       Check that the SVG files use only the LIGHT colors of palette.txt.
#       The colors of the properties fill, stroke, stop-color, flood-color,
#       lighting-color and color are checked. Their values must be "none",
#       "url(...)" or a LIGHT color of palette.txt.
#   awk -v mode=dark -f palette.awk palette.txt FILE.svg
#       Print FILE.svg with the LIGHT colors replaced by the DARK colors.
#
# Errors are printed to the standard error as "FILE:LINE: message", and the
# exit status becomes 1.
#
# This script is used by generate.sh and test/check_icons.sh.
# It uses only the features of POSIX awk (no interval expressions such as
# {6}, which some awk implementations do not support).

function error( file, line, msg )
{
    printf( "%s:%d: %s\n", file, line, msg ) > "/dev/stderr"
    status = 1
}

function is_color( s )
{
    return s ~ /^#[0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f]$/
}

BEGIN {
    status = 0
    n_colors = 0
    if ( mode != "palette" && mode != "check" && mode != "dark" )
    {
        print "palette.awk: mode must be palette, check or dark" > "/dev/stderr"
        status = 2
        exit
    }
}

# palette.txt (the first file)
FILENAME == ARGV[1] {
    if ( NF == 0 || $1 ~ /^#/ )
    {
        next
    }
    if ( NF != 3 )
    {
        error( FILENAME, FNR, "expected three fields: ROLE LIGHT DARK" )
        next
    }
    if ( ! is_color( $2 ) || ! is_color( $3 ) )
    {
        error( FILENAME, FNR, "colors must be written as #rrggbb in lower case" )
        next
    }
    if ( $1 in role_line )
    {
        error( FILENAME, FNR, "duplicated role: " $1 )
        next
    }
    if ( $2 in dark )
    {
        error( FILENAME, FNR, "duplicated light color: " $2 )
        next
    }
    role_line[$1] = FNR
    dark[$2] = $3
    ++n_colors
    next
}

# SVG files: check the colors of the properties
mode == "check" {
    rest = $0
    # the property name must not follow a letter, '-', '_' or ':'
    # (e.g. "stroke-width", "pagecolor" and "inkscape:deskcolor" are not checked)
    while ( match( rest, /(^|[^-A-Za-z_:])(fill|stroke|stop-color|flood-color|lighting-color|color)[ \t]*[:=][ \t]*["']?[^;"' \t>]*/ ) )
    {
        prop = substr( rest, RSTART, RLENGTH )
        rest = substr( rest, RSTART + RLENGTH )

        value = prop
        sub( /^[^:=]*[:=][ \t]*["']?/, "", value )
        name = prop
        sub( /[ \t]*[:=].*$/, "", name )
        sub( /^[^A-Za-z]/, "", name )
        lvalue = tolower( value )

        if ( lvalue == "none" || lvalue ~ /^url\(/ )
        {
            continue
        }
        if ( ! is_color( lvalue ) )
        {
            error( FILENAME, FNR, name ": \"" value "\" is not a color of the form #rrggbb" )
        }
        else if ( ! ( lvalue in dark ) )
        {
            error( FILENAME, FNR, name ": " value " is not in palette.txt" )
        }
    }
    next
}

# SVG file: replace the light colors with the dark colors
mode == "dark" {
    rest = $0
    out = ""
    while ( match( rest, /#[0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f]/ ) )
    {
        token = substr( rest, RSTART, 7 )
        after = substr( rest, RSTART + 7, 1 )
        key = tolower( token )
        out = out substr( rest, 1, RSTART - 1 )
        if ( after !~ /[0-9A-Za-z_]/ && ( key in dark ) )
        {
            out = out dark[key]
        }
        else
        {
            out = out token
        }
        rest = substr( rest, RSTART + 7 )
    }
    print out rest
    next
}

END {
    if ( status == 0 && n_colors == 0 )
    {
        print "palette.awk: no color is defined in " ARGV[1] > "/dev/stderr"
        status = 1
    }
    exit status
}
