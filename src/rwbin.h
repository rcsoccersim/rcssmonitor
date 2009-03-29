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
#ifndef RWBIN_H
#define RWBIN_H

#include <iostream>

#include <sys/types.h>

#ifndef NATIVE_INT_FORMAT
#include <netinet/in.h>
#endif

inline
void
rbin( std::istream & in,
      char & val )
{
    in.read( &val, sizeof( char ) );
}

inline
void
rbin( std::istream & in,
      signed char & val )
{
    in.read( reinterpret_cast< char * >( &val ), sizeof( signed char ) );
}

inline
void
rbin( std::istream & in,
      unsigned char & val )
{
    in.read( reinterpret_cast< char * >( &val ), sizeof( unsigned char ) );
}

inline
void
rbin( std::istream & in,
      signed int & val )
{
#ifdef NATIVE_INT_FORMAT
    in.read( reinterpret_cast< char * >( &val ), sizeof( signed int ) );
#else
    unsigned long nval;
    in.read( reinterpret_cast< char * >( &nval ), sizeof( unsigned long ) );
    val = static_cast< signed int >( ntohl( nval ) );
#endif
}

inline
void
rbin( std::istream & in,
      signed long & val )
{
#ifdef NATIVE_INT_FORMAT
    in.read( reinterpret_cast< char * >( &val ), sizeof( signed long ) );
#else
    unsigned long nval;
    in.read( reinterpret_cast< char * >( &nval ), sizeof( unsigned long ) );
    val = static_cast< signed long >( ntohl( nval ) );
#endif
}

inline
void
rbin( std::istream & in,
      signed short & val )
{
#ifdef NATIVE_INT_FORMAT
    in.read( reinterpret_cast< char * >( &val ), sizeof( signed short ) );
#else
    unsigned short nval;
    in.read( reinterpret_cast< char * >( &nval ), sizeof( unsigned short ) );
    val = static_cast< signed short >( ntohs( nval ) );
#endif
}

inline
void
rbin( std::istream & in,
      unsigned int & val )
{
#ifdef NATIVE_INT_FORMAT
    in.read( reinterpret_cast< char * >( &val ), sizeof( unsigned int ) );
#else
    unsigned long nval;
    in.read( reinterpret_cast< char * >( &nval ), sizeof( unsigned long ) );
    val = static_cast< unsigned int >( ntohl( nval ) );
#endif
}

inline
void
rbin( std::istream & in,
      unsigned long & val )
{
#ifdef NATIVE_INT_FORMAT
    in.read( reinterpret_cast< char * >( &val ), sizeof( unsigned long ) );
#else
    unsigned long nval;
    in.read( reinterpret_cast< char * >( &nval ), sizeof( unsigned long ) );
    val = ntohl( nval );
#endif
}

inline
void
rbin( std::istream & in,
      unsigned short & val )
{
#ifdef NATIVE_INT_FORMAT
    in.read( reinterpret_cast< char * >( &val ), sizeof( unsigned short ) );
#else
    unsigned short nval;
    in.read( reinterpret_cast< char * >( &nval ), sizeof( unsigned short ) );
    val = ntohs( nval );
#endif
}

inline
void
rbin( std::istream & in,
      bool & val )
{
#ifdef NATIVE_INT_FORMAT
    in.read( reinterpret_cast< char * >( &val ), sizeof( bool ) );
#else
    unsigned char charCast;
    in.read( reinterpret_cast< char * >( &charCast ), sizeof( unsigned char ) );
    val = static_cast< bool >( charCast );
#endif
}

inline
void
rbin( std::istream & in,
      float & val )
{
    in.read( reinterpret_cast< char * >( &val ), sizeof( float ) );
}

inline
void
rbin( std::istream & in,
      double & val )
{
    in.read( reinterpret_cast< char * >( &val ), sizeof( double ) );
}

inline
void
rbin( std::istream & in,
      long double & val )
{
    in.read( reinterpret_cast< char * >( &val ), sizeof( long double ) );
}

/******************************************************************************/

inline
void
wbin( std::ostream & out,
      signed char val )
{
    out.write( reinterpret_cast< const char * >( &val ), sizeof( signed char ) );
}

inline
void
wbin( std::ostream & out,
      char val )
{
    out.write( reinterpret_cast< const char * >( &val ), sizeof( char ) );
}

inline
void
wbin( std::ostream & out,
      unsigned char val )
{
    out.write( reinterpret_cast< const char * >( &val ), sizeof( unsigned char ) );
}

inline
void
wbin( std::ostream & out,
      signed int val )
{
#ifdef NATIVE_INT_FORMAT
    out.write( reinterpret_cast< const char * >( &val ), sizeof( signed int ) );
#else
    unsigned long nval = htonl ( val );
    out.write( reinterpret_cast< const char * >( &nval ), sizeof( unsigned long ) );
#endif
}

inline
void
wbin( std::ostream & out,
      signed long val )
{
#ifdef NATIVE_INT_FORMAT
    out.write( reinterpret_cast< const char * >( &val ), sizeof( signed long ) );
#else
    unsigned long nval = htonl ( val );
    out.write( reinterpret_cast< const char * >( &nval ), sizeof( unsigned long ) );
#endif
}

inline
void
wbin( std::ostream & out,
      signed short val )
{
#ifdef NATIVE_INT_FORMAT
    out.write( reinterpret_cast< const char * >( &val ), sizeof( signed short ) );
#else
    unsigned short nval = htons ( val );
    out.write( reinterpret_cast< const char * >( &nval ), sizeof( unsigned short ) );
#endif
}

inline
void
wbin( std::ostream & out,
      unsigned int val )
{
#ifdef NATIVE_INT_FORMAT
    out.write( reinterpret_cast< const char * >( &val ), sizeof( unsigned int ) );
#else
    unsigned long nval = htonl ( val );
    out.write( reinterpret_cast< const char * >( &nval ), sizeof( unsigned long ) );
#endif
}

inline
void
wbin( std::ostream & out,
      unsigned long val )
{
#ifdef NATIVE_INT_FORMAT
    out.write( reinterpret_cast< const char * >( &val ), sizeof( unsigned long ) );
#else
    unsigned long nval = htonl ( val );
    out.write( reinterpret_cast< const char * >( &nval ), sizeof( unsigned long ) );
#endif
}

inline
void wbin( std::ostream & out,
           unsigned short val )
{
#ifdef NATIVE_INT_FORMAT
    out.write( reinterpret_cast< const char * >( &val ), sizeof( unsigned short ) );
#else
    unsigned short nval = htons ( val );
    out.write( reinterpret_cast< const char * >( &nval ), sizeof( unsigned short ) );
#endif
}

inline
void
wbin( std::ostream & out,
      bool val )
{
#ifdef NATIVE_INT_FORMAT
    out.write( reinterpret_cast< const char * >( &val ), sizeof( bool ) );
#else
    unsigned char charCast = ( unsigned char ) val;
    out.write( reinterpret_cast< const char * >( &charCast ), sizeof( unsigned char ) );
#endif
}

inline
void
wbin( std::ostream & out,
      float val )
{
    out.write( reinterpret_cast< const char * >( &val ), sizeof( float ) );
}

inline
void
wbin( std::ostream & out,
      double val )
{
    out.write( reinterpret_cast< const char * >( &val ), sizeof( double ) );
}

inline
void
wbin( std::ostream & out,
      long double val )
{
    out.write( reinterpret_cast< const char * >( &val ), sizeof( long double ) );
}

#endif


#if 0
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/*************      T E S T     ***************************************/
#include <fstream>

main()
{
    char c = 'C', c2;
    double d = 3.14, d2;
    int i = -254, i2;


    ofstream out( "xxx.tmp" );
    wbin( out, c );
    wbin( out, d );
    wbin( out, i );
    out.close();

    ifstream in( "xxx.tmp" );
    rbin( in, c2 );
    rbin( in, d2 );
    rbin( in, i2 );
    in.close();

    ofstream out2( "xxx2.tmp" );
    wbin( out2, c2 );
    wbin( out2, d2 );
    wbin( out2, i2 );
    out2.close();

    return 1;
}

#endif
