// -*-c++-*-

/*!
  \file angle_deg.cpp
  \brief Degree wrapper class Source File.
*/

/*
 *Copyright:

 Copyright (C) Hidehisa Akiyama

 This code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "angle_deg.h"

#ifndef M_PI
//! PI value macro
#define M_PI 3.14159265358979323846
#endif

const double AngleDeg::EPSILON = 1.0e-5;

const double AngleDeg::DEG2RAD = M_PI / 180.0;
const double AngleDeg::RAD2DEG = 180.0 / M_PI;
