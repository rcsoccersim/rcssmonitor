/*
 * Copyright (c) 1999 - 2001, Artur Merke <amerke@ira.uka.de>
 *
 * This file is part of FrameView2d.
 *
 * FrameView2d is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * FrameView2d is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FrameView2d; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "global_defs.h"
#include "ascii_processor.h"
#include "str2val.h"
#include <cstdlib>
#include <cstdio>

#define P_ERROR(xxx) ERROR_OUT << "\nparse error (" << __LINE__ << "): " << xxx
#define P_WARNING(xxx) WARNING_OUT << "\nparse warning (" << __LINE__ << "): " << xxx
#define P_MSG(xxx) //cerr << "\nparse msg: " << xxx

const char AsciiProcessor::DELIMITER_CHAR = ';';

Multi< Point2d >     AsciiProcessor::points( 20 );
Multi< Line2d >      AsciiProcessor::lines( 10 );
Multi< Circle2d >    AsciiProcessor::circles( 10 );
Multi< CircleArc2d > AsciiProcessor::circlearcs( 10 );
Multi< char >        AsciiProcessor::characters( 100 );

const bool AsciiProcessor::CMD_ERR =  false;
const bool AsciiProcessor::CMD_OK  =  true;


bool
AsciiProcessor::mov_cmd( const char * buf,
                         BuilderBase * build,
                         const char* & next )
{
    int fref;
    Point2d pos;
    Angle ang;

    if ( ! str2val( buf, fref, next ) )
    {
        P_ERROR( "MOV no frame reference specified" );
        return CMD_ERR;
    }

    buf = next;

    int res = get_frame( buf, pos, ang, next );

    if ( res < 1 )
    {
        return CMD_ERR;
    }

    if ( ! strskip( next, DELIMITER_CHAR, next ) )
    {
        return CMD_ERR;
    }

    switch ( res )
    {

    case 3:
        build->set_cmd_set_frame_pos_ang( fref, pos, ang );
        P_MSG( "build->set_cmd_set_frame_pos_ang(fref,(" << pos.x << "," << pos.y << ")," << ang.get_value() << ");" );
        break;

    case 2:
        build->set_cmd_set_frame_pos( fref, pos );
        P_MSG( "build->set_cmd_set_frame_pos(fref,pos)" );
        break;

    case 1:
        build->set_cmd_set_frame_ang( fref, ang );
        P_MSG( "build->set_cmd_set_frame_ang(fref,ang);" );
        break;
    }

    return CMD_OK;
}

bool
AsciiProcessor::ins_obj( const char * buf,
                         int fref,
                         BuilderBase * build,
                         const char* & next )
{
    // ==========================================================
    if ( strskip( buf, "FRAME", next ) )
    {
        buf = next;
        return ins_frame( buf, fref, build, next );
    }

    // ==========================================================
    else if ( strskip( buf, "CIRCLE", next ) )
    {
        buf = next;
        return ins_circle( buf, fref, build, next );
    }

    // ==========================================================

    else if ( strskip( buf, "LINE", next ) )
    {
        buf = next;
        return ins_line( buf, fref, build, next );
    }

    // ==========================================================

    else if ( strskip( buf, "POINT", next ) )
    {
        buf = next;
        return ins_point( buf, fref, build, next );
    }

    // ==========================================================

    else if ( strskip( buf, "POLYLINE", next ) )
    {
        buf = next;
        return ins_polyline( buf, fref, build, next );
    }

    // ==========================================================

    else if ( strskip( buf, "POLYGON", next ) )
    {
        buf = next;
        return ins_polygon( buf, fref, build, next );
    }

    // ==========================================================

    else if ( strskip( buf, "STRING", next ) )
    {
        buf = next;
        return ins_string( buf, fref, build, next );
    }

    // ==========================================================

    else if ( strskip( buf, "GRID", next ) )
    {
        buf = next;
        return ins_grid( buf, fref, build, next );
    }

    // ==========================================================

    else if ( strskip( buf, "STR_GRID", next ) )
    {
        buf = next;
        return ins_string_grid( buf, fref, build, next );
    }

    else
    {
        return CMD_ERR;
    }

    return CMD_OK;
}

bool
AsciiProcessor::visual_area_cmd( const char * buf,
                                 BuilderBase * build,
                                 const char* & next )
{
    Area2d a;
    double d1;
    int res = get_max5_tuple( buf, 4,
                              a.center.x, a.center.y,
                              a.size_x, a.size_y,
                              d1,
                              next );

    if ( res != 4 )
    {
        P_ERROR( "SETAREA wrong area specification, retval= " << res );
        return CMD_ERR;
    }

    buf = next;

    if ( ! strskip( buf, DELIMITER_CHAR, next ) )
    {
        P_ERROR( "wrong delimiter char" );
        return CMD_ERR;
    }

    P_MSG( "build->set_cmd_set_view_area(area[(" << a.center.x << "," << a.center.y << ")," << a.size_x << "," << a.size_y << "]" << ");" );

    build->set_cmd_set_view_area( a );

    return CMD_OK;
}

bool
AsciiProcessor::status_line_cmd( const char * buf,
                                 BuilderBase * build,
                                 const char * & next )
{
    int res = get_string( buf, characters, next );

    if ( res < 0 )
    {
        P_ERROR( "SL wrong text specification, retval= " << res );
        return CMD_ERR;
    }

    buf = next;

    if ( ! strskip( buf, DELIMITER_CHAR, next ) )
    {
        P_ERROR( "wrong delimiter char" );
        return CMD_ERR;
    }

    P_MSG( "build->set_cmd_set_status(characters);" );

    build->set_cmd_set_status_line( characters.cur_size, characters.tab );

    return CMD_OK;
}

bool
AsciiProcessor::background_color_cmd( const char * buf,
                                      BuilderBase * build,
                                      const char* & next )
{
    RGBcolor col;

    if ( ! get_col( buf, col, next ) )
    {
        P_ERROR( "wrong color [" << buf << "]" );
        return false;
    }

    buf = next;

    if ( ! strskip( buf, DELIMITER_CHAR, next ) )
    {
        P_ERROR( "wrong delimiter char" );
        return CMD_ERR;
    }

    build->set_cmd_set_background_color( col );

    return CMD_OK;
}

bool
AsciiProcessor::del_cmd( const char * buf,
                         BuilderBase * build,
                         const char* & next )
{
    int fref;

    if ( ! str2val( buf, fref, next ) )
    {
        P_ERROR( "DEL no frame or object reference" );
        return CMD_ERR;
    }

    buf = next;

    int oref;

    if ( str2val( buf, oref, next ) )
    {
        buf = next;

        if ( ! strskip( buf, DELIMITER_CHAR, next ) )
        {
            P_ERROR( "wrong delimiter char" );
            return CMD_ERR;
        }

        build->set_cmd_remove_object( fref, oref );

        P_MSG( "build->set_cmd_remove_object(fref,oref);" );
        return CMD_OK;
    }

    if ( ! strskip( buf, DELIMITER_CHAR, next ) )
    {
        P_ERROR( "wrong delimiter char" );
        return CMD_ERR;
    }

    build->set_cmd_remove_frame( fref );

    P_MSG( "build->set_cmd_remove_frame(fref);" );

    return CMD_OK;
}

bool
AsciiProcessor::emp_cmd( const char * buf,
                         BuilderBase * build,
                         const char * & next )
{
    int fref;

    if ( ! str2val( buf, fref, next ) )
    {
        P_ERROR( "EMP no frame reference" );
        return CMD_ERR;
    }

    buf = next;

    if ( ! strskip( buf, DELIMITER_CHAR, next ) )
    {
        P_ERROR( "wrong delimiter char" );
        return CMD_ERR;
    }

    build->set_cmd_empty_frame( fref );

    P_MSG( "build->set_cmd_empty_frame(" << fref << ");" );

    return CMD_OK;
}

bool
AsciiProcessor::visible_cmd( const char * buf,
                             BuilderBase * build,
                             const char * & next,
                             int visible )
{
    int fref;

    if ( ! str2val( buf, fref, next ) )
    {
        P_ERROR( "no frame reference" );
        return CMD_ERR;
    }

    buf = next;

    if ( ! strskip( buf, DELIMITER_CHAR, next ) )
    {
        P_ERROR( "wrong delimiter char" );
        return CMD_ERR;
    }

    P_MSG( "build->set_cmd_set_frame_visible(" << fref << ");" );

    if ( ! build->set_cmd_set_frame_visible( fref, visible ) )
    {
        P_ERROR( "wrong frame reference" );
    }

    return CMD_OK;
}

bool
AsciiProcessor::scan_and_parse( BuilderBase * build,
                                const char * buf )
{
    while ( buf )
    {
        while ( *buf == ' ' || *buf == '\n' || *buf == '\t' )
        {
            buf++;
        }

        if ( *buf == '\0' )
        {
            break;
        }

        const char * next = buf;

        bool res = false;

        int fref = 0;

        switch ( buf[0] )
        {
            //------------------------------------------------------------------------

        case 'B':

            if ( buf[1] == 'G' )
            {
                if ( background_color_cmd( buf + 2, build, next ) )
                {
                    res = true;
                    buf = next;
                    //cout << "col set" << flush;
                }
            }

            break;

            //------------------------------------------------------------------------

        case 'M':

            if ( buf[1] == 'O' && buf[2] == 'V' )
            {
                if ( mov_cmd( buf + 3, build, next ) )
                {
                    res = true;
                    buf = next;
                }
            }

            break;

            //------------------------------------------------------------------------

        case  'S':

            if ( buf[1] == 'L' )
            {
                if ( status_line_cmd( buf + 2, build, next ) )
                {
                    res = true;
                    buf = next;
                }
            }
            else if ( buf[1] == 'H' && buf[2] == 'O' && buf[3] == 'W' )
            {
                if ( visible_cmd( buf + 4, build, next, 1 ) )
                {
                    res = true;
                    buf = next;
                    //cout << "\nSHOW";
                }
            }

            break;

            //------------------------------------------------------------------------

        case  'H':

            if ( buf[1] == 'I' && buf[2] == 'D' && buf[3] == 'E' )
            {
                if ( visible_cmd( buf + 4, build, next, 0 ) )
                {
                    res = true;
                    buf = next;
                }
            }

            break;

            //------------------------------------------------------------------------

        case  'D':

            if ( buf[1] == 'E' && buf[2] == 'L' )
            {
                if ( del_cmd( buf + 3, build, next ) )
                {
                    res = true;
                    buf = next;
                }
            }

            break;

            //------------------------------------------------------------------------

        case  'E':

            if ( buf[1] == 'M' && buf[2] == 'P' )
            {
                if ( emp_cmd( buf + 3, build, next ) )
                {
                    res = true;
                    buf = next;
                }
            }

            break;

            //------------------------------------------------------------------------

        case 'I':

            if ( buf[1] == 'N' && buf[2] == 'S' )
            {
                buf += 3;
                fref = 0;

                if ( str2val( buf, fref, next ) )
                    buf = next;

                if ( ins_obj( buf, fref, build, next ) )
                {
                    res = true;
                    buf = next;
                }
            }

            break;

            //------------------------------------------------------------------------

        case  'V':

            if ( buf[1] == 'A' )
                if ( visual_area_cmd( buf + 2, build, next ) )
                {
                    res = true;
                    buf = next;
                }

            break;

            //------------------------------------------------------------------------
        }

        if ( !res )
        {
            P_ERROR( "unrecognized command "
                     << "\nmsg= [" << buf << "]"
                     << "\nerr= [" << next << "]" );
            return false;//break;
        }

        //P_MSG("buf [" << buf << "]");
    }

    return true;
}

bool
AsciiProcessor::ins_frame( const char * buf,
                           int fref,
                           BuilderBase * build,
                           const char * & next )
{
    int id = 0, lay = 0, vis = 1;
    Point2d pos;
    Angle ang;

    if ( ! get_frame_attr( buf, id, lay, vis, next ) )
    {
        return CMD_ERR;
    }

    int res = get_frame( next, pos, ang, next );

    if ( res < -1 )
    {
        return CMD_ERR;
    }

    if ( ! strskip( next, DELIMITER_CHAR, next ) )
    {
        return CMD_ERR;
    }

    build->set_cmd_insert_frame( fref, id, pos, ang, lay );

    return CMD_OK;
}

bool
AsciiProcessor::ins_point( const char * buf,
                           int fref,
                           BuilderBase * build,
                           const char * & next )
{
    int id = 0, lay = 0, fil = 0;
    RGBcolor col( 0, 0, 0 );

    if ( ! get_obj_attr( buf, id, lay, col, fil, next ) )
    {
        return CMD_ERR;
    }

    int num = get_points( next, points, next );

    if ( num < 1 )
    {
        return CMD_ERR;
    }

    if ( num == 1 )
    {
        build->set_cmd_insert_point( fref, id, points.tab[0], lay, col );
    }
    else
    {
        build->set_cmd_insert_points( fref, id, points.cur_size, points.tab, lay, col );
    }

    return CMD_OK;
}

bool
AsciiProcessor::ins_line( const char * buf,
                          int fref,
                          BuilderBase * build,
                          const char * & next )
{
    int id = 0, lay = 0, fil = 0;
    RGBcolor col( 0, 0, 0 );

    if ( ! get_obj_attr( buf, id, lay, col, fil, next ) )
    {
        return CMD_ERR;
    }

    int num = get_lines( next, lines, next );

    if ( num < 1 )
    {
        return CMD_ERR;
    }

    if ( num == 1 )
    {
        build->set_cmd_insert_line( fref, id, lines.tab[0], lay, col );
    }
    else
    {
        build->set_cmd_insert_lines( fref, id, lines.cur_size, lines.tab, lay, col );
    }

    return CMD_OK;
}

bool
AsciiProcessor::ins_circle( const char * buf,
                            int fref,
                            BuilderBase * build,
                            const char * & next )
{
    int id = 0, lay = 0, fil = 0;
    RGBcolor col( 0, 0, 0 );

    if ( ! get_obj_attr( buf, id, lay, col, fil, next ) )
    {
        return CMD_ERR;
    }

    int num = get_circles( next, circles, next );

    if ( num < 1 )
    {
        return CMD_ERR;
    }

    if ( num == 1 )
    {
        if ( ! fil )
        {
            build->set_cmd_insert_circle( fref, id, circles.tab[0], lay, col );
        }
        else
        {
            build->set_cmd_insert_f_circle( fref, id, circles.tab[0], lay, col );
        }
    }
    else
    {
        if ( !fil )
        {
            build->set_cmd_insert_circles( fref, id, circles.cur_size, circles.tab, lay, col );
        }
        else
        {
            build->set_cmd_insert_f_circles( fref, id, circles.cur_size, circles.tab, lay, col );
        }
    }

    return CMD_OK;
}

bool
AsciiProcessor::ins_polygon( const char * buf,
                             int fref,
                             BuilderBase * build,
                             const char * & next )
{
    int id = 0, lay = 0, fil = 0;
    RGBcolor col( 0, 0, 0 );

    if ( ! get_obj_attr( buf, id, lay, col, fil, next ) )
    {
        return CMD_ERR;
    }

    int num = get_points( next, points, next );

    if ( num < 2 )
    {
        return CMD_ERR;
    }

    if ( !fil )
    {
        build->set_cmd_insert_polygon( fref, id, points.cur_size, points.tab, lay, col );
    }
    else
    {
        build->set_cmd_insert_f_polygon( fref, id, points.cur_size, points.tab, lay, col );
    }

    return CMD_OK;
}

bool
AsciiProcessor::ins_polyline( const char * buf,
                              int fref,
                              BuilderBase * build,
                              const char * & next )
{
    int id = 0, lay = 0, fil = 0;
    RGBcolor col( 0, 0, 0 );

    if ( ! get_obj_attr( buf, id, lay, col, fil, next ) )
    {
        return CMD_ERR;
    }

    int num = get_points( next, points, next );

    if ( num < 2 )
    {
        return CMD_ERR;
    }

    build->set_cmd_insert_polyline( fref, id, points.cur_size, points.tab, lay, col );

    return CMD_OK;
}

bool
AsciiProcessor::ins_string( const char * buf,
                            int fref,
                            BuilderBase * build,
                            const char * & next )
{
    int id = 0, lay = 0, fil = 0;
    RGBcolor col( 0, 0, 0 );

    Point2d point;

    if ( ! get_obj_attr( buf, id, lay, col, fil, next ) )
    {
        return CMD_ERR;
    }

    while ( true ) //accept more then one string!
    {
        if ( ! strskip( next, '(', next ) )
        {
            return CMD_ERR;
        }

        if ( ! str2val( next, point.x, next ) )
        {
            return CMD_ERR;
        }

        if ( ! strskip( next, ',', next ) )
        {
            return CMD_ERR;
        }

        if ( ! str2val( next, point.y, next ) )
        {
            return CMD_ERR;
        }

        if ( ! strskip( next, ',', next ) )
        {
            return CMD_ERR;
        }

        buf = next;

        int offset;

        int res = get_string( buf, offset, next );

        if ( res < 0 )
        {
            return CMD_ERR;
        }

        if ( ! strskip( next, ')', next ) )
        {
            return CMD_ERR;
        }

        VisualObject2d * string2d = new VisualString2d( id, lay, col, point, res, buf + offset );

        build->set_cmd_insert_visobject( fref, string2d );

        if ( strskip( next, DELIMITER_CHAR, next ) )
        {
            break;
        }
    }

    return CMD_OK;
}

bool
AsciiProcessor::ins_grid( const char * buf,
                          int fref,
                          BuilderBase * build,
                          const char * & next )
{
    int id = 0, lay = 0, fil = 0;
    RGBcolor col( 0, 0, 0 );

    if ( ! get_obj_attr( buf, id, lay, col, fil, next ) )
    {
        return CMD_ERR;
    }

    int idx = 0;

    while ( true ) //accept more then one grid!
    {
        double p_x, p_y, v1_x, v1_y, v2_x, v2_y, d3, d4, d5;
        int num_v1 = 1, num_v2 = 1;

        if ( ! strskip( next, '(', next ) )
        {
            return CMD_ERR;
        }

        if ( get_max5_tuple( next, 2, p_x, p_y, d3, d4, d5, next ) != 2 )
        {
            return CMD_ERR;
        }

        if ( get_max5_tuple( next, 2, v1_x, v1_y, d3, d4, d5, next ) != 2 )
        {
            return CMD_ERR;
        }

        if ( strskip( next, ':', next ) )
        {
            if ( ! str2val( next, num_v1, next ) )
            {
                return CMD_ERR;
            }
        }

        if ( get_max5_tuple( next, 2, v2_x, v2_y, d3, d4, d5, next ) != 2 )
        {
            return CMD_ERR;
        }

        if ( strskip( next, ':', next ) )
        {
            if ( ! str2val( next, num_v2, next ) )
            {
                return CMD_ERR;
            }
        }

        if ( ! strskip( next, ')', next ) )
        {
            return CMD_ERR;
        }

        lines.set_cur_size( idx );

        lines.set_max_size_and_preserve_cur_size( idx + num_v1 + num_v2 + 2 );

        const double  d1 = 1.0 / double( num_v1 );

        const double  d2 = 1.0 / double( num_v2 );

        for ( int i = 0; i < num_v2 + 1; ++i )
        {
            Line2d & l = lines.tab[idx];
            l.p1.x = p_x + i * v2_x * d2 ;
            l.p1.y = p_y + i * v2_y * d2 ;
            l.p2.x = l.p1.x + v1_x;
            l.p2.y = l.p1.y + v1_y;
            idx++;
        }

        for ( int i = 0; i < num_v1 + 1; ++i )
        {
            Line2d & l = lines.tab[idx];
            l.p1.x = p_x + i * v1_x * d1 ;
            l.p1.y = p_y + i * v1_y * d1 ;
            l.p2.x = l.p1.x + v2_x;
            l.p2.y = l.p1.y + v2_y;
            idx++;
        }

        if ( strskip( next, DELIMITER_CHAR, next ) )
        {
            break;
        }
    }

    build->set_cmd_insert_lines( fref, id, idx, lines.tab, lay, col );

    return CMD_OK;
}

bool
AsciiProcessor::ins_string_grid( const char * buf,
                                 int fref,
                                 BuilderBase * build,
                                 const char * & next )
{
    int id = 0, lay = 0, fil = 0;
    RGBcolor col( 0, 0, 0 );

    if ( ! get_obj_attr( buf, id, lay, col, fil, next ) )
    {
        return CMD_ERR;
    }

    while ( true )
    {
        double p_x, p_y, v1_x, v1_y, v2_x, v2_y, d3, d4, d5;
        int num_v1 = 1, num_v2 = 1;

        if ( ! strskip( next, '(', next ) )
        {
            return CMD_ERR;
        }

        if ( get_max5_tuple( next, 2, p_x, p_y, d3, d4, d5, next ) != 2 )
        {
            return CMD_ERR;
        }

        if ( get_max5_tuple( next, 2, v1_x, v1_y, d3, d4, d5, next ) != 2 )
        {
            return CMD_ERR;
        }

        if ( strskip( next, ':', next ) )
        {
            if ( ! str2val( next, num_v1, next ) )
            {
                return CMD_ERR;
            }
        }

        if ( get_max5_tuple( next, 2, v2_x, v2_y, d3, d4, d5, next ) != 2 )
        {
            return CMD_ERR;
        }

        if ( strskip( next, ':', next ) )
        {
            if ( ! str2val( next, num_v2, next ) )
            {
                return CMD_ERR;
            }
        }

        const double  d1 = 1.0 / double( num_v1 );

        const double  d2 = 1.0 / double( num_v2 );

        bool early_stop = false;

        for ( int i = 0; i < num_v2; ++i )
        {
            Point2d point;
            point.x = p_x + i * v2_x * d2 ;
            point.y = p_y + i * v2_y * d2 ;

            for ( int j = 0; j < num_v1; j++ )
            {
                int offset;

                if ( strskip( next, ')', next ) )
                {
                    early_stop = true;
                    break;
                }

                buf = next;

                int res = get_string( buf, offset, next );

                if ( res < 0 )
                {
                    return CMD_ERR;
                }

                VisualObject2d * string2d = new VisualString2d( id, lay, col, point, res, buf + offset );

                build->set_cmd_insert_visobject( fref, string2d );

                point.x += v1_x * d1;
                point.y += v1_y * d1;
            }

            if ( early_stop )
            {
                break;
            }
        }

        if ( ! early_stop )
        {
            if ( ! strskip( next, ')', next ) )
            {
                return CMD_ERR;
            }
        }

        if ( strskip( next, DELIMITER_CHAR, next ) )
        {
            break;
        }
    }

    return CMD_OK;
}

bool
AsciiProcessor:: get_obj_attr( const char * buf,
                               int & id,
                               int & lay,
                               RGBcolor & col,
                               int & fil,
                               char const * & next )
{
    next = buf;
    bool got_attr[4] = { false, false, false, false };

    while ( next )
    {
        if ( strskip( buf, "id", next ) )
        {
            if ( got_attr[0] )
            {
                return false;
            }

            got_attr[0] = true;

            buf = next;

            if ( ! strskip( buf, "=", next ) )
            {
                return false;
            }

            buf = next;

            if ( ! str2val( buf, id, next ) )
            {
                return false;
            }

            buf = next;
        }
        else if ( strskip( buf, "lay", next ) )
        {
            if ( got_attr[1] )
            {
                return false;
            }

            got_attr[1] = true;

            buf = next;

            if ( ! strskip( buf, "=", next ) )
            {
                return false;
            }

            buf = next;

            if ( ! str2val( buf, lay, next ) )
            {
                return false;
            }

            buf = next;
        }
        else if ( strskip( buf, "col", next ) )
        {
            if ( got_attr[2] )
            {
                return false;
            }

            got_attr[2] = true;

            buf = next;

            if ( ! strskip( buf, "=", next ) )
            {
                return false;
            }

#if 0
            buf = next;

            if ( ! strskip( buf, "#", next ) )
            {
                return false;
            }

#endif
            buf = next;

            if ( ! get_col( buf, col, next ) )
            {
                P_ERROR( "wrong color [" << buf << "]" );
                return false;
            }

            buf = next;

            //if ( !strconv(buf,col,next) )
            //return false;
            buf = next;
        }
        else if ( strskip( buf, "fil", next ) )
        {
            if ( got_attr[3] )
            {
                return false;
            }

            got_attr[3] = true;

            buf = next;

            if ( ! strskip( buf, "=", next ) )
            {
                return false;
            }

            buf = next;

            bool b;

            if ( ! str2val( buf, b, next ) )
            {
                return false;
            }

            fil = b;

            buf = next;
        }
        else
        {
            next = buf;
            return true;
        }
    }

    next = buf;

    return true;
}

bool
AsciiProcessor:: get_frame_attr( const char * buf,
                                 int & id,
                                 int & lay,
                                 int & vis,
                                 char const * & next )
{
    next = buf;
    bool got_attr[4] = { false, false, false, false };

    while ( next )
    {
        if ( strskip( buf, "id", next ) )
        {
            if ( got_attr[0] )
            {
                return false;
            }

            got_attr[0] = true;

            buf = next;

            if ( ! strskip( buf, "=", next ) )
            {
                return false;
            }

            buf = next;

            if ( ! str2val( buf, id, next ) )
            {
                return false;
            }

            buf = next;
        }
        else if ( strskip( buf, "lay", next ) )
        {
            if ( got_attr[1] )
            {
                return false;
            }

            got_attr[1] = true;

            buf = next;

            if ( ! strskip( buf, "=", next ) )
            {
                return false;
            }

            buf = next;

            if ( ! str2val( buf, lay, next ) )
            {
                return false;
            }

            buf = next;
        }
        else if ( strskip( buf, "vis", next ) )
        {
            bool b;

            if ( got_attr[1] )
            {
                return false;
            }

            got_attr[1] = true;

            buf = next;

            if ( ! strskip( buf, "=", next ) )
            {
                return false;
            }

            buf = next;

            if ( ! str2val( buf, b, next ) )
            {
                return false;
            }

            vis = b;

            buf = next;
        }
        else
        {
            next = buf;
            return true;
        }
    }

    next = buf;

    return true;
}

bool
AsciiProcessor::get_col( const char * buf,
                         RGBcolor & col,
                         const char * & next )
{
    while ( *buf == ' ' )
    {
        ++buf;
    }

    if ( buf == '\0' )
    {
        return false;
    }

    unsigned char c[3];

    bool res = false;

    for ( int i = 0; i < 3; ++i )
    {
        if ( buf[0] != '\0' && buf[1] != '\0' )
        {
            char chr[3];
            chr[0] = buf[0];
            chr[1] = buf[1];
            chr[2] = '\0';
            char* error_ptr;
            long tmp_long = strtol( chr, &error_ptr, 16 );

            if ( error_ptr != chr + 2 || tmp_long < 0 || tmp_long > 255 )
            {
                //cerr << "\ntmp_long=" << tmp_long;
                break;
            }

            c[i] = ( unsigned char ) tmp_long;

            buf += 2;

            if ( i == 2 )
            {
                res = true;
            }
        }
        else
        {
            break;
        }
    }

    if ( res == true )
    {
        next = buf;
        col = RGBcolor( c[0], c[1], c[2] );
        return true;
    }

    return false;
}

/*!

  \retval 0 a empty tuple was received "()"
  \retval 1 a one tuple "( <num> )"
  \retval 2,3,4,5 analogous with 1
  \retval -1: the was no tuple at all, i.e. the string doesn't begin with a '('
  \retval -2: some failure occured
*/
int
AsciiProcessor::get_max5_tuple( const char * buf,
                                int num,
                                double & d0,
                                double & d1,
                                double & d2,
                                double & d3,
                                double & d4,
                                char const * & next )
{
    if ( ! strskip( buf, '(', next ) )
    {
        return -1;
    }

    buf = next;

    if ( ! str2val( buf, d0, next ) )
    {
        if ( strskip( buf, ')', next ) )
        {
            return 0;
        }

        return -2;
    }

    if ( num < 1 ) return -2;

    buf = next;

    if ( ! strskip( buf, ',', next ) )
    {
        if ( strskip( buf, ')', next ) )
        {
            return 1;
        }

        return -2;
    }

    if ( num < 2 ) return -2;

    buf = next;

    if ( ! str2val( buf, d1, next ) )
    {
        return -2;
    }

    buf = next;

    if ( !strskip( buf, ',', next ) )
    {
        if ( strskip( buf, ')', next ) )
        {
            return 2;
        }

        return -2;
    }

    if ( num < 3 ) return -2;

    buf = next;

    if ( !str2val( buf, d2, next ) )
    {
        return -2;
    }

    buf = next;

    if ( ! strskip( buf, ',', next ) )
    {
        if ( strskip( buf, ')', next ) )
        {
            return 3;
        }

        return -2;
    }

    if ( num < 4 ) return -2;

    buf = next;

    if ( ! str2val( buf, d3, next ) )
    {
        return -2;
    }

    buf = next;

    if ( ! strskip( buf, ',', next ) )
    {
        if ( strskip( buf, ')', next ) )
        {
            return 4;
        }

        return -2;
    }

    if ( num < 5 ) return -2;

    buf = next;

    if ( ! str2val( buf, d4, next ) )
    {
        return -2;
    }

    buf = next;

    if ( ! strskip( buf, ')', next ) )
    {
        return -2;
    }

    return 5;
}

int
AsciiProcessor::get_frame( const char * buf,
                           Point2d & p,
                           Angle & a,
                           char const * & next )
{
    double d0, d1, d2, d3, d4;
    int res = get_max5_tuple( buf, 3, d0, d1, d2, d3, d4, next );

    if ( res == 2 )
    {
        p.x = d0;
        p.y = d1;
    }

    if ( res == 3 )
    {
        p.x = d0;
        p.y = d1;
        a = Angle( d2 );
    }

    if ( res == 1 )
    {
        a = Angle( d0 );
    }

    return res;
}

int
AsciiProcessor::get_point( const char * buf,
                           Point2d & p,
                           char const * & next )
{
    double d2, d3, d4;
    return get_max5_tuple( buf, 2, p.x, p.y, d2, d3, d4, next );
}

int
AsciiProcessor::get_line( const char * buf,
                          Line2d & l,
                          char const * & next )
{
    double d4;
    return get_max5_tuple( buf, 4, l.p1.x, l.p1.y, l.p2.x, l.p2.y, d4, next );
}

int
AsciiProcessor::get_circle( const char * buf,
                            Circle2d & c,
                            char const * & next )
{
    double d3, d4;
    return get_max5_tuple( buf, 3, c.center.x, c.center.y, c.radius, d3, d4, next );
}

int
AsciiProcessor::get_circlearc( const char * buf,
                               CircleArc2d & c,
                               char const* & next )
{
    double d3, d4;
    int res = get_max5_tuple( buf, 5, c.center.x, c.center.y, c.radius, d3, d4, next );

    if ( res > 3 )
    {
        c.ang1 = Angle( d3 );
    }

    if ( res > 4 )
    {
        c.ang2 = Angle( d4 );
    }

    return res;
}

int
AsciiProcessor::get_points( const char * buf,
                            Multi< Point2d > & points,
                            char const* & next )
{
    int res;
    int k = 0;

    while ( 1 )
    {
        if ( k >= points.max_size )
        {
            points.set_cur_size( k ); //do not loose the points
            points.set_max_size_and_preserve_cur_size( k + k + 1 );
        }

        res = get_point( buf, points.tab[k], next );

        if ( res == -1 )
        {
            break;
        }

        if ( res != 2 )
        {
            return -1;
        }

        buf = next;

        k++;
    };

    if ( ! strskip( buf, DELIMITER_CHAR, next ) )
    {
        P_ERROR( "wrong delimiter char" );
        return -1;
    }

    points.set_cur_size( k );

    return k;
}

int
AsciiProcessor::get_lines( const char * buf,
                           Multi< Line2d > & lines,
                           char const * & next )
{
    int res;
    int k = 0;

    while ( 1 )
    {
        if ( k >= lines.max_size )
        {
            lines.set_cur_size( k ); //do not loose entries
            lines.set_max_size_and_preserve_cur_size( k + k + 1 );
        }

        res = get_line( buf, lines.tab[k], next );

        if ( res == -1 )
        {
            break;
        }

        if ( res != 4 )
        {
            return -1;
        }

        buf = next;

        ++k;
    };

    if ( ! strskip( buf, DELIMITER_CHAR, next ) )
    {
        P_ERROR( "wrong delimiter char" );
        return -1;
    }

    lines.set_cur_size( k );

    return k;
}

int
AsciiProcessor::get_circles( const char * buf,
                             Multi< Circle2d > & circles,
                             char const * & next )
{
    int res;
    int k = 0;

    while ( 1 )
    {
        if ( k >= circles.max_size )
        {
            circles.set_cur_size( k ); //do not loose entries
            circles.set_max_size_and_preserve_cur_size( k + k + 1 );
        }

        res = get_circle( buf, circles.tab[k], next );

        if ( res == -1 )
        {
            break;
        }

        if ( res != 3 )
        {
            return -1;
        }

        buf = next;

        ++k;
    };

    if ( ! strskip( buf, DELIMITER_CHAR, next ) )
    {
        P_ERROR( "wrong delimiter char" );
        return -1;
    }

    circles.set_cur_size( k );

    return k;
}

int
AsciiProcessor::get_circlearcs( const char * buf,
                                Multi< CircleArc2d > & circles,
                                char const* & next )
{
    int res;
    int k = 0;

    while ( 1 )
    {
        if ( k >= circles.max_size )
        {
            circles.set_cur_size( k ); //do not loose entries
            circles.set_max_size_and_preserve_cur_size( k + k + 1 );
        }

        res = get_circlearc( buf, circles.tab[k], next );

        if ( res == -1 )
        {
            break;
        }

        if ( res != 5 && res != 3 )
        {
            return -1;
        }

        buf = next;

        ++k;
    };

    if ( ! strskip( buf, DELIMITER_CHAR, next ) )
    {
        P_ERROR( "wrong delimiter char" );
        return -1;
    }

    circles.set_cur_size( k );

    return k;
}

int
AsciiProcessor::get_string( const char * buf,
                            Multi< char > & str,
                            char const * & next )
{
    if ( ! strskip( buf, "\"", next ) )
    {
        return -1;
    }

    buf = next;

    int k = 0;

    while ( *buf !=  '\"' && *buf != '\0' )
    {
        if ( k >= str.max_size )
        {
            str.set_cur_size( k ); //do not loose entries
            str.set_max_size_and_preserve_cur_size( k + k + 1 );
        }

        str.tab[k] = *buf;

        ++buf;
        ++k;
    }

    if ( *buf != '\"' )
    {
        return -2;
    }

    ++buf;

    next = buf;

    str.set_cur_size( k );

    return k;
}

int
AsciiProcessor::get_string( const char * buf,
                            int & offset,
                            char const * & next )
{
    if ( ! strskip( buf, "\"", next ) )
    {
        return -1;
    }

    offset = next - buf;

    buf = next;

    int k = 0;

    while ( *buf !=  '\"' && *buf != '\0' )
    {
        ++buf;
        ++k;
    }

    if ( *buf != '\"' )
    {
        return -2;
    }

    ++buf;

    next = buf;

    return k;
}
