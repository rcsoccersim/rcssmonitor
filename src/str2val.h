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
#ifndef _STR2VAL_H_
#define _STR2VAL_H_

/* finds the next occurence of chr, returns 0 if chr was not there */
int strfind(char const* str,char chr, char const* & next);

int strskip(char const* str,char chr, char const* & next);
inline int strskip(char const* str,char chr) {
  const char * dum;
  return strskip(str,chr,dum);
}

/*skips just the expression, regardless of what follows in str, this is
 different to the earlier version of strskip */
int strskip(char const* str,char const* expr, char const* & next);
inline int strskip(char const* str,char const* expr) {
  const char * dum;
  return strskip(str,expr,dum);
}

/*tests if this string contains just white space until ist end*/
int strempty(char const* str, char const* & next);
inline int strempty(char const* str) {
  const char * dum;
  return strempty(str,dum);
}

/*tests if this string contains at least 1 white space char at its begin*/
int strspace(char const* str, char const* & next);
int strspace(char const* str);

/*the value of the specific type is recognized as far as possible*/
int str2val(char const* str,    int & val, char const* & next);
int str2val(char const* str,   long & val, char const* & next);
int str2val(char const* str, double & val, char const* & next);
int str2val(char const* str,  float & val, char const* & next);
int str2val(char const* str,   bool & val, char const* & next);

inline int str2val(char const* str,    int & val) {
  const char * dum;
  return str2val(str, val, dum);
}
inline int str2val(char const* str,   long & val) {
  const char * dum;
  return str2val(str, val, dum);
}
inline int str2val(char const* str, double & val) {
  const char * dum;
  return str2val(str, val, dum);
}
inline int str2val(char const* str,  float & val) {
  const char * dum;
  return str2val(str, val, dum);
}
inline int str2val(char const* str,   bool & val) {
  const char * dum;
  return str2val(str, val, dum);
}

int str2val(char const* str, int num,    int * val, char const* & next);
int str2val(char const* str, int num,   long * val, char const* & next);
int str2val(char const* str, int num, double * val, char const* & next);
int str2val(char const* str, int num,  float * val, char const* & next);
int str2val(char const* str, int num,   bool * val, char const* & next);

inline int str2val(char const* str, int num,    int * val) {
  const char * dum;
  return str2val(str, num, val, dum);
}
inline int str2val(char const* str, int num,   long * val) {
  const char * dum;
  return str2val(str, num, val, dum);
}
inline int str2val(char const* str, int num, double * val) {
  const char * dum;
  return str2val(str, num, val, dum);
}
inline int str2val(char const* str, int num,  float * val) {
  const char * dum;
  return str2val(str, num, val, dum);
}
inline int str2val(char const* str, int num,   bool * val) {
  const char * dum;
  return str2val(str, num, val, dum);
}

#endif
