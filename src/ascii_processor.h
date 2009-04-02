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

#ifndef ASCII_PROCESSOR_H
#define ASCII_PROCESSOR_H

#include "multi.h"

class Angle;
class Point2d;
class Line2d;
class Circle2d;
class CircleArc2d;
class BuilderBase;
class RGBcolor;

/*
  <root> ::= <cmd> <root>

  <cmd> ::= DEL <int>;
  ::= EMP <int>;
  ::= INS {<int>}+ <obj>;
  ::= MOV <int> <frame_data>;
  ::= ST  <string>;
  ::= VA  <va_data>;
  ::= SHOW <int>;
  ::= HIDE <int>;

  <obj> ::= FRAME     <frame_attr> <frame_data>
  ::= POINT     <obj_atrr>   <point_data>
  ::= LINE      <obj_attr>   <line_data>
  ::= CIRCLE    <obj_attr>   <circle_data>
  ::= CIRCLEARC <obj_attr>   <circlearc_data>
  ::= POLYLINE  <obj_attr>   <point_data>
  ::= POLYGON   <obj_attr>   <point_data>

  <obj_attr> ::=

  ...

*/

class AsciiProcessor {
private:
    static const char DELIMITER_CHAR;

    static Multi< Point2d > points;
    static Multi< Line2d > lines;
    static Multi< Circle2d > circles;
    static Multi< CircleArc2d > circlearcs;
    static Multi< char > characters;

    static int get_max5_tuple( const char * buf,
                               int num,
                               double & d0,
                               double & d1,
                               double & d2,
                               double & d3,
                               double & d4,
                               char const * & next );

    static int get_frame( const char * buf,
                          Point2d & p,
                          Angle & a,
                          char const * & next );

    static int get_point( const char * buf,
                          Point2d & p,
                          char const * & next );

    static int get_line( const char * buf,
                         Line2d & p,
                         char const * & next );

    static int get_circle( const char * buf,
                           Circle2d & p,
                           char const * & next );

    static int get_circlearc( const char * buf,
                              CircleArc2d & p,
                              char const * & next );

    static int get_points( const char * buf,
                           Multi< Point2d > & pp,
                           char const * & next );

    static int get_lines( const char * buf,
                          Multi< Line2d > & pp,
                          char const * & next );

    static int get_circles( const char * buf,
                            Multi< Circle2d > & pp,
                            char const * & next );

    static int get_circlearcs( const char * buf,
                               Multi< CircleArc2d > & pp,
                               char const * & next );

    static int get_string( const char * buf,
                           Multi< char > & pp,
                           char const * & next );

    static int get_string( const char * buf,
                           int & offset,
                           char const * & next );

    static bool get_obj_attr( const char * buf,
                              int & id,
                              int & lay,
                              RGBcolor & col,
                              int & fil,
                              char const * & next );

    static bool get_frame_attr( const char * buf,
                                int & id,
                                int & lay,
                                int & vis,
                                char const * & next );

    static const bool CMD_ERR;
    static const bool CMD_OK;

    static bool mov_cmd( const char * buf,
                         BuilderBase * build,
                         const char * & next );

    static bool visual_area_cmd( const char * buf,
                                 BuilderBase * build,
                                 const char * & next );

    static bool status_line_cmd( const char * buf,
                                 BuilderBase * build,
                                 const char * & next );

    static bool background_color_cmd( const char * buf,
                                      BuilderBase * build,
                                      const char * & next );

    static bool del_cmd( const char * buf,
                         BuilderBase * build,
                         const char * & next );

    static bool emp_cmd( const char * buf,
                         BuilderBase * build,
                         const char * & next );

    static bool visible_cmd( const char * buf,
                             BuilderBase * build,
                             const char * & next,
                             int visible );

public:
    static bool get_col( const char * buf,
                         RGBcolor & col,
                         const char * & next );

    static bool get_col( const char * buf,
                         RGBcolor & col )
      {
          const char * tmp;
          return get_col( buf, col, tmp );
      }

    static bool scan_and_parse( BuilderBase * build,
                                const char * buf );

    static bool ins_obj( const char * buf,
                         int fref,
                         BuilderBase * build,
                         const char * & next );

    static bool ins_frame( const char * buf,
                           int fref,
                           BuilderBase * build,
                           const char * & next );

    static bool ins_grid( const char * buf,
                          int fref,
                          BuilderBase * build,
                          const char * & next );

    static bool ins_string_grid( const char * buf,
                                 int fref,
                                 BuilderBase * build,
                                 const char * & next );

    static bool ins_point( const char * buf,
                           int fref,
                           BuilderBase * build,
                           const char * & next );

    static bool ins_line( const char * buf,
                          int fref,
                          BuilderBase * build,
                          const char * & next );

    static bool ins_circle( const char * buf,
                            int fref,
                            BuilderBase * build,
                            const char * & next );

    static bool ins_string( const char * buf,
                            int fref,
                            BuilderBase * build,
                            const char * & next );

    static bool ins_polygon( const char * buf,
                             int fref,
                             BuilderBase * build,
                             const char * & next );

    static bool ins_polyline( const char * buf,
                              int fref,
                              BuilderBase * build,
                              const char * & next );
};

#endif
