/*
 * Copyright (c) 1999 - 2001, Artur Merke <amerke@ira.uka.de> 
 *
 * This code is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * It is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "angle.h"
#include <cmath>

const double Angle::TwoPi = 2*M_PI;

double cos(const Angle &a) {
  return std::cos(a.ang);
}

double sin(const Angle &a) {
  return std::sin(a.ang);
}

#if 0
ostream& operator<< (ostream& o,const Angle &a) {
  return o << a.ang;
}

istream& operator>> (istream& i,Angle &a) {
  cout << "\n istream& operator>> (istream& i,Angle &a)";
  return i >> a.ang;
}
#endif

Angle operator+(const Angle &a1, const Angle &a2) {
  Angle res;
  res.ang= a1.ang+a2.ang;
  if (res.ang >= Angle::TwoPi) 
    res.ang-= Angle::TwoPi;
  return res;
}

Angle operator-(const Angle &a1, const Angle &a2) {
  Angle res;
  res.ang= a1.ang-a2.ang;
  if (res.ang < 0.0) 
    res.ang+= Angle::TwoPi;
  return res;
}

Angle operator-(const Angle &a1) {
  Angle res= a1;
  res.ang= -res.ang + Angle::TwoPi;
  return res;
}

void Angle::operator +=(const Angle & a) {
  ang+= a.ang;
  if (ang >= TwoPi) 
    ang-= TwoPi;
}

void Angle::operator -=(const Angle & a) {
  ang-= a.ang;
  if (ang < 0.0) 
    ang+= TwoPi;
}

Angle::Angle() {
  ang= 0.0;
}

Angle::Angle( const Angle& a) {
  ang= a.ang;
}

Angle::Angle( double d) {
  set_value(d);
}

void Angle::set_value( double d) {
  ang= fmod(d,TwoPi);
  if (ang < 0.0) 
    ang+= TwoPi;
}
