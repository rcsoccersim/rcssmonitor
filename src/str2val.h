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
#ifndef STR2VAL_H
#define STR2VAL_H

/*!
  \brief finds the next occurence of 'chr'
  \param str input string
  \param chr target character
  \param next reference the pointer variable that indicates next position.
  \retval 1 if chr was found.
  \retval 0 if chr was not there.
*/
int
strfind( char const * str,
         char chr,
         char const * & next );

/*!
  \brief skip white spaces and the character 'chr'
  \param str input string
  \param chr target character
  \param next reference the pointer variable that indicates next position.
  \retval 1 if 'chr' was skipped.
  \retval 0 if 'chr' was not there.
 */
int
strskip( char const * str,
         char chr,
         char const * & next );

/*!
  \brief another version of strskip, provided for convenience
 */
inline
int
strskip( char const * str,
         char chr )
{
    const char * dum;
    return strskip( str, chr, dum );
}

/*!
  \brief skips just the expression, regardless of what follows in str, this is
  different to the earlier version of strskip
  \param str input string
  \param expression target expression string
  \param next reference the pointer variable that indicates next position.
  \retval 1 if 'expr' was skipped.
  \retval 1 if 'expr' was not there.
*/
int
strskip( char const * str,
         char const * expr,
         char const * & next );

/*!
  \brief another version of strskip, provided for convenience
 */
inline
int
strskip( char const * str,
         char const * expr )
{
    const char * dum;
    return strskip( str, expr, dum );
}

/*
  \brief tests if this string contains just white space until ist end
  \retval 1 if 'str' contains only white spaces
  \retval 0 if 'str' contains some characthers.
*/
int
strempty( char const * str,
          char const * & next );

/*!
  \brief another version of strempty, provided for convenience
 */
inline
int
strempty( char const * str )
{
    const char * dum;
    return strempty( str, dum );
}

/*!
  \brief tests if this string contains at least 1 white space char at its begin
  \param str input string
  \param next reference the pointer variable that indicates next position.
  \retval 1 if white space was found at beginning.
  \retval 0 if white space was not there.
*/
int
strspace( char const * str,
          char const * & next );

/*!
  \brief another version of strspace, provided for convenience
 */
int
strspace( char const * str );

/*!
  \brief parse integer value using std::strtol() and check errors.
  \param str input string
  \param val reference to the result variable
  \param next reference the pointer variable that indicates next position.
  \retval 1 if value was parsed successfully.
  \retval 0 if value could not be parsed.
*/
int
str2val( char const * str,
         int & val,
         char const* & next );

/*!
  \brief parse integer value using std::strtol() and check errors.
  \param str input string
  \param val reference to the result variable
  \param next reference the pointer variable that indicates next position.
  \retval 1 if value was parsed successfully.
  \retval 0 if value could not be parsed.
*/
int
str2val( char const * str,
         long & val,
         char const * & next );

/*!
  \brief parse double value using std::strtod() and check errors.
  \param str input string
  \param val reference to the result variable
  \param next reference the pointer variable that indicates next position.
  \retval 1 if value was parsed successfully.
  \retval 0 if value could not be parsed.
*/
int
str2val( char const * str,
         double & val,
         char const* & next );

/*!
  \brief parse float value using std::strtod() and check errors.
  \param str input string
  \param val reference to the result variable
  \param next reference the pointer variable that indicates next position.
  \retval 1 if value was parsed successfully.
  \retval 0 if value could not be parsed.
*/
int
str2val( char const * str,
         float & val,
         char const * & next );

/*!
  \brief parse boolean value and check errors.
  \param str input string
  \param val reference to the result variable
  \param next reference the pointer variable that indicates next position.
  \retval 1 if value was parsed successfully.
  \retval 0 if value could not be parsed.

  true values: '1' or "true"
  false values: '0' or "false"
*/
int
str2val( char const * str,
         bool & val,
         char const * & next );

/*!
  \brief another version of str2val for interger value, provided for convenience
 */
inline
int
str2val( char const * str,
         int & val )
{
    const char * dum;
    return str2val( str, val, dum );
}

/*!
  \brief another version of str2val for long interger value, provided for convenience
 */
inline
int
str2val( char const * str,
         long & val )
{
    const char * dum;
    return str2val( str, val, dum );
}

/*!
  \brief another version of str2val for double value, provided for convenience
 */
inline
int
str2val( char const * str,
         double & val )
{
    const char * dum;
    return str2val( str, val, dum );
}

/*!
  \brief another version of str2val for float value, provided for convenience
 */
inline
int
str2val( char const * str,
         float & val )
{
    const char * dum;
    return str2val( str, val, dum );
}

/*!
  \brief another version of str2val for boolean value, provided for convenience
 */
inline
int
str2val( char const * str,
         bool & val )
{
    const char * dum;
    return str2val( str, val, dum );
}

/*!
  \brief parse interger values and set them to the result array variable.
  \param str input string
  \param num the size of result array variable
  \param val the result array variable
  \param next reference the pointer variable that indicates next position.
  \return the number of successfully parsed values.
 */
int
str2val( char const * str,
         int num,
         int * val,
         char const* & next );

/*!
  \brief parse long interger values and set them to the result array variable.
  \param str input string
  \param num the size of result array variable
  \param val the result array variable
  \param next reference the pointer variable that indicates next position.
  \return the number of successfully parsed values.
 */
int
str2val( char const * str,
         int num,
         long * val,
         char const * & next );

/*!
  \brief parse double values and set them to the result array variable.
  \param str input string
  \param num the size of result array variable
  \param val the result array variable
  \param next reference the pointer variable that indicates next position.
  \return the number of successfully parsed values.
 */
int
str2val( char const * str,
         int num,
         double * val,
         char const * & next );

/*!
  \brief parse float values and set them to the result array variable.
  \param str input string
  \param num the size of result array variable
  \param val the result array variable
  \param next reference the pointer variable that indicates next position.
  \return the number of successfully parsed values.
 */
int
str2val( char const * str,
         int num,
         float * val,
         char const * & next );

/*!
  \brief parse boolean values and set them to the result array variable.
  \param str input string
  \param num the size of result array variable
  \param val the result array variable
  \param next reference the pointer variable that indicates next position.
  \return the number of successfully parsed values.
 */
int
str2val( char const * str,
         int num,
         bool * val,
         char const * & next );

/*!
  \brief another version of str2val for integer values, provided for convenience
 */
inline
int
str2val( char const * str,
         int num,
         int * val )
{
    const char * dum;
    return str2val( str, num, val, dum );
}

/*!
  \brief another version of str2val for long integer values, provided for convenience
 */
inline
int
str2val( char const * str,
         int num,
         long * val )
{
    const char * dum;
    return str2val( str, num, val, dum );
}

/*!
  \brief another version of str2val for double values, provided for convenience
 */
inline
int
str2val( char const * str,
         int num,
         double * val )
{
    const char * dum;
    return str2val( str, num, val, dum );
}

/*!
  \brief another version of str2val for float values, provided for convenience
 */
inline
int
str2val( char const * str,
         int num,
         float * val )
{
    const char * dum;
    return str2val( str, num, val, dum );
}

/*!
  \brief another version of str2val for boolean values, provided for convenience
 */
inline
int
str2val( char const * str,
         int num,
         bool * val )
{
    const char * dum;
    return str2val( str, num, val, dum );
}

#endif
