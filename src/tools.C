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
#include "tools.h"
#include "global_defs.h"
#include <sys/time.h>

long TOOLS::get_current_ms_time()
{
    timeval tval;
    static long s_time_at_start = 0;

    if ( gettimeofday( &tval, NULL ) )
    {
        ERROR_OUT << "\n something wrong with time mesurement";
    }

    if ( 0 == s_time_at_start )
    {
        s_time_at_start = tval.tv_sec;
    }

    return ( tval.tv_sec - s_time_at_start ) * 1000 + tval.tv_usec / 1000;
}
