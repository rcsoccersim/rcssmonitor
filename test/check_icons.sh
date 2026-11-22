#!/bin/sh
#
# check_icons.sh -- check the icon files in icons/
#
# Usage: check_icons.sh [ICONS_DIR]
#
# ICONS_DIR is the icons directory of the source tree (default: the icons
# directory next to the directory of this script).
#
# The following points are checked. Inkscape is not needed.
#   - palette.txt has the correct format.
#   - the SVG files in svg/ use only the LIGHT colors of palette.txt.
#   - each icon (svg/NAME.svg) has the PNG files light/SIZE/NAME.png and
#     dark/SIZE/NAME.png of the sizes 16, 24, 32 and 48 (SIZExSIZE pixels).
#   - each application icon (app/svg/NAME.svg) has the PNG files
#     app/SIZE/NAME.png of the sizes 16, 32, 48, 64, 128 and 256, and
#     app/svg/NAME.SIZE.svg (the version for a size) is for one of the sizes.
#   - no PNG file exists without the SVG file.
#   - icons.qrc and icons.am list exactly these files.
#   - the PNG files have been created from the current SVG files and
#     palette.txt (the checksums recorded in generated.sha256 by
#     generate.sh match the files).
#
# If a check fails, run icons/generate.sh to create the PNG files and the
# lists again (see docs/icons.md).
#
# Exit status: 0 = pass, 1 = fail, 99 = error (Automake convention).

# messages and sorting do not depend on the locale
LC_ALL=C
export LC_ALL

icons_dir="${1:-$(dirname "$0")/../icons}"
if [ ! -f "$icons_dir/palette.txt" ] || [ ! -f "$icons_dir/palette.awk" ]; then
    echo "error: $icons_dir is not the icons directory" >&2
    exit 99
fi
cd "$icons_dir" || exit 99

SIZES="16 24 32 48"
APP_SIZES="16 32 48 64 128 256"
result=0

fail()
{
    echo "FAIL: $*" >&2
    result=1
}

# print the width and height of a PNG file ("WIDTH HEIGHT"), or nothing if
# the file is not a PNG file
png_size()
{
    # the signature (8 bytes) and the IHDR chunk header (8 bytes) are
    # followed by the width and the height (4 bytes each, big endian)
    od -An -tu1 -N24 "$1" 2>/dev/null | awk '
        { for ( i = 1; i <= NF; ++i ) b[++n] = $i }
        END {
            if ( n == 24 && b[1] == 137 && b[2] == 80 && b[3] == 78 && b[4] == 71 \
                 && b[13] == 73 && b[14] == 72 && b[15] == 68 && b[16] == 82 )
            {
                printf( "%d %d\n", ((b[17] * 256 + b[18]) * 256 + b[19]) * 256 + b[20],
                                   ((b[21] * 256 + b[22]) * 256 + b[23]) * 256 + b[24] )
            }
        }'
}

# palette.txt
if ! awk -v mode=palette -f palette.awk palette.txt; then
    fail "errors in palette.txt"
fi

# the icons
names=""
for f in svg/*.svg; do
    [ -f "$f" ] || continue
    names="$names $(basename "$f" .svg)"
done
if [ -z "$names" ]; then
    fail "no SVG file in svg/"
fi

# the application icons (app/svg/NAME.svg) and the versions for the sizes
# (app/svg/NAME.SIZE.svg)
app_names=""
app_svg_files=""
for f in app/svg/*.svg; do
    [ -f "$f" ] || continue
    app_svg_files="$app_svg_files $f"
    case "$(basename "$f" .svg)" in
        *.*)
            base=$(basename "$f" .svg | cut -d . -f 1)
            size=$(basename "$f" .svg | cut -d . -f 2-)
            [ -f "app/svg/$base.svg" ] || fail "$f: app/svg/$base.svg does not exist"
            echo " $APP_SIZES " | grep -q " $size " || fail "$f: the size must be one of $APP_SIZES"
            ;;
        *)
            app_names="$app_names $(basename "$f" .svg)"
            ;;
    esac
done

# names of the icons
for name in $names $app_names; do
    case "$name" in
        *[!a-z0-9_]*) fail "invalid icon name: $name (use only a-z, 0-9 and _)" ;;
    esac
done
for name in $app_names; do
    [ ! -f "svg/$name.svg" ] && continue
    fail "$name is used by both svg/ and app/svg/"
done

# colors of the SVG files
svg_files=$(for name in $names; do echo "svg/$name.svg"; done)
# shellcheck disable=SC2086
if ! awk -v mode=check -f palette.awk palette.txt $svg_files; then
    fail "the SVG files must use only the LIGHT colors of palette.txt"
fi

# the expected PNG files
expected_png=$( (for name in $names; do
                     for scheme in light dark; do
                         for size in $SIZES; do
                             echo "$scheme/$size/$name.png"
                         done
                     done
                 done
                 for name in $app_names; do
                     for size in $APP_SIZES; do
                         echo "app/$size/$name.png"
                     done
                 done) | sort)

# existence and sizes of the PNG files
for png in $expected_png; do
    if [ ! -f "$png" ]; then
        fail "$png does not exist"
        continue
    fi
    size=$(echo "$png" | cut -d / -f 2) # light/SIZE/NAME.png, dark/SIZE/NAME.png, app/SIZE/NAME.png
    actual=$(png_size "$png")
    if [ -z "$actual" ]; then
        fail "$png is not a PNG file"
    elif [ "$actual" != "$size $size" ]; then
        fail "$png: the size is $(echo "$actual" | tr ' ' x) (expected ${size}x$size)"
    fi
done

# PNG files without the SVG file
actual_png=$(for f in light/*/*.png dark/*/*.png app/*/*.png; do
                 [ -f "$f" ] && echo "$f"
             done | sort)
for png in $actual_png; do
    if ! echo "$expected_png" | grep -qx "$png"; then
        case "$png" in
            app/*) fail "$png has no SVG file (app/svg/$(basename "$png" .png).svg) or is not of the sizes $APP_SIZES" ;;
            *) fail "$png has no SVG file (svg/$(basename "$png" .png).svg) or is not of the sizes $SIZES" ;;
        esac
    fi
done

# compare_list NAME EXPECTED ACTUAL [COMMAND]
#   compare the lists of the files (one file per line). COMMAND is the
#   command to fix it (default: icons/generate.sh --lists).
compare_list()
{
    if [ "$2" != "$3" ]; then
        fail "$1 does not list the expected files. Run ${4:-icons/generate.sh --lists}."
        EXPECTED="$2" ACTUAL="$3" awk 'BEGIN {
            n = split( ENVIRON["EXPECTED"], e, "\n" )
            for ( i = 1; i <= n; ++i ) if ( e[i] != "" ) expected[e[i]] = 1
            n = split( ENVIRON["ACTUAL"], a, "\n" )
            for ( i = 1; i <= n; ++i ) if ( a[i] != "" ) actual[a[i]] = 1
            for ( f in expected ) if ( ! ( f in actual ) ) print "  missing: " f
            for ( f in actual ) if ( ! ( f in expected ) ) print "  extra:   " f
        }' >&2
    fi
}

# icons.qrc
if [ -f icons.qrc ]; then
    qrc_files=$(sed -n 's|.*<file>\(.*\)</file>.*|\1|p' icons.qrc | sort)
    compare_list icons.qrc "$expected_png" "$qrc_files"
else
    fail "icons.qrc does not exist"
fi

# icons.am
if [ -f icons.am ]; then
    expected_am=$( (echo "$names" | tr ' ' '\n' | sed '/^$/d; s|.*|svg/&.svg|'
                    echo "$app_svg_files" | tr ' ' '\n' | sed '/^$/d'
                    echo "$expected_png") | sort)
    am_files=$(sed -n 's|^[[:space:]]*\$(ICONS_DIR)/\([^[:space:]]*\).*|\1|p' icons.am | sort)
    compare_list icons.am "$expected_am" "$am_files"
else
    fail "icons.am does not exist"
fi

# generated.sha256
if command -v sha256sum >/dev/null 2>&1; then
    sha256="sha256sum"
elif command -v shasum >/dev/null 2>&1; then
    sha256="shasum -a 256"
else
    sha256=""
    echo "SKIP: the checksums are not checked (sha256sum or shasum is not found)"
fi
if [ -n "$sha256" ]; then
    if [ ! -f generated.sha256 ]; then
        fail "generated.sha256 does not exist. Run icons/generate.sh."
    else
        expected_sum=$( (echo palette.txt; echo "$svg_files"
                         echo "$app_svg_files" | tr ' ' '\n' | sed '/^$/d') | sort)
        recorded_sum=$(sed 's/^[0-9a-f]*  //' generated.sha256 | sort)
        compare_list generated.sha256 "$expected_sum" "$recorded_sum" icons/generate.sh
        # print only the files that do not match
        changed=$($sha256 -c generated.sha256 2>/dev/null | grep -v ': OK$')
        if [ -n "$changed" ]; then
            fail "the following files have been changed after the PNG files were created. Run icons/generate.sh."
            echo "$changed" | sed 's/^/  /' >&2
        fi
    fi
fi

if [ "$result" -eq 0 ]; then
    echo "PASS: $(echo "$names" | wc -w | tr -d ' ') icons and $(echo "$app_names" | wc -w | tr -d ' ') application icons in $icons_dir"
fi
exit "$result"
