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
#ifndef VALUEPARSER_H
#define VALUEPARSER_H

//#include <string>
#include <iostream>

/**
   Ein einfacher Zeilenparser, der Eintraege der Form

   <string> = <werte>

   einlesen kann.
   Als <werte> koennen folgende Eintraege vorkommen

   <werte> ::= <int>    | <int>*
   <werte> ::= <long>   | <long>*
   <werte> ::= <float>  | <float>*
   <werte> ::= <double> | <double>*
   <werte> ::= <string>
   <werte> ::= <bool>

   Der Zugriff auf solche Zeilen erfolgt mittels der typisierten Methoden
   get(const char *,int&);
   get(const char *,long&); ... usw.

   Es finden keine Fehlerkorrekturen der Eingabe statt. Z.B ist 2.1 keine gueltige
   Eingabe fuer ein int!!!


   Wir betrachten dazu als Beispiel die Methode
   get(const char *,int&) mit dem Aufruf
   get("key",i)

   Beispiele fuer Eintraege in der Datei und Interpretationen sind

   key= 123            -> i= 123
   key= 123  124       -> i= 123              (Warnung)
   key= 3.9            -> i= <undefiniert>    (Warnung)
   key=                -> i= <undefiniert>    (Warnung)
   key= 7a             -> i= <undefiniert>    (Warnung)
   key= a              -> i= <undefiniert>    (Warnung)
   key= 11111111111111 -> i= <undefiniert>    (Warnung da zu gross)

   Von den Typen int,long,float,double,bool,char koennen auch Arrays eingelesen werden.
   Die uebergebenen Feleder muessen bereist genuegend Speicher fuer alle Eintraege
   reserviert haben.

   Auch hierzu ein Paar Beispiele fuer die Methode

   get(const char *,int* value, int len) mit dem Aufruf
   get("key",a,2);

   ( Das Feld a muss dazu durch  int a[k]; oder int *a= new int[k]; (k>=2) o.a.
   Speicher reserviert haben. )

   key= 123 124            -> a= [123,124]
   key= 123 124 124        -> a= [123,124]              (Warnung)
   key= 123                -> a= [123,<undefiniert>]    (Warnung)
   key= 11111111111111 123 -> a= [<undefiniert>,<undefiniert>]  (Warnung da erster Wert zu gross)

   key= 3.1 123            -> a= [<undefiniert>, <undefiniert>]  (Warnung)
   ".1 123" wird als ungueltige Eigabe betrachtet

   key=                    -> a= [<undefiniert>, <undefiniert>] (Warnung)


   Zu jeder Methode gibt es auch eine Variante, die einen default Wert setzen kann.
   Dieser werden als Parameter uebergeben.

   Anmerkungen zur Implementierung:

   Die jetzige Implementierung ist nicht sehr effizient, reicht aber locker fuer
   Datein mit weniger als ein Paar Hundert Eintraegen. Es wird keine Hashtabelle
   verwendet, und der Aufwand fuer das Einlesen eines Bezeichners ist linear in der
   Anzahl aller Eintraege.

   Die maximale Anzahl der Eintraege muss zur Compilierungszeit festgelegt werden,
   und ist durch die  Konstante "maxNumEntries" festgelegt.

   Eine spaetere Implementierung sollte Hashtabellen verwenden, und keine festen
   Grenzen fuer die Anzahl der Eintraege und die max. Zeilenlaenge voraussetzen!

   Dem Benutzer kann das alles egal sein, da sich die Schnittstelle nach aussen
   durch veraenderte Implementierungen nicht aendern wird.

   Autor: Artur Merke
*/

struct KeyValueTab {
    int max_size;
    int cur_size;

    struct Entry {
        Entry()
          {
              access = 0;
              key = 0;
              val = 0;
          }

        int  access;
        char * key;
        char * val;
    };

    Entry * tab;

    KeyValueTab();
    KeyValueTab( int new_max_size );
    ~KeyValueTab();

    /// i must be between 0 <= i < cur_size
    bool set_key( int i, const char * key,
                  int len = -1 );
    bool set_last_key( const char * key,
                       int len = -1 )
      {
          return set_key( cur_size -1, key, len );
      }

    /// i must be between 0 <= i < cur_size
    bool set_val( int i,
                  const char * value,
                  int len = -1 );
    bool set_last_val( const char * value,
                       int len = -1 )
      {
          return set_val( cur_size -1, value, len );
      }


    /** i must be between 0 <= i < cur_size
        \param sep if true, then a separating ' ' is included if existing value is != 0;
    */
    bool append_val( int i,
                     const char * value,
                     bool sep );

    ///sets new cur_size, uses set_max_size if necessary
    bool set_cur_size( int size );
    bool inc_cur_size()
      {
          return set_cur_size( cur_size + 1 );
      }

    ///enlarges the table, cur_size remains the same (incl. the entries)
    bool set_max_size( int size );
};

class ValueParser {
protected:
    // Members
    KeyValueTab kv_tab;
    int max_key_length; //just for better output

    enum {
        type_int = 0,        // type constants
        type_long = 1,
        type_float = 2,
        type_double = 3,
        type_string = 4,
        type_bool = 5,
        type_char = 6,
        //static const int type_last=6;        // Must cont. highest type const.
    };

    int getValue( const char * key,
                  int type,
                  void * value,
                  int length = 1 ) const;
    void showValue( const char * key,
                    int type,
                    void* value,
                    int length = 1 ) const;
    bool verbose_mode;
    bool warnings_mode;

    // Constants
    static const int maxLineLength;  // Maximal line length

    bool read_line( const char * line );

public:
    /// Es werden alle Bezeichner innerhalbs eines Blocks [block]
    /// eingelesen. Ist block ein String von Laenge 0 (=Voreinstellung)
    /// so werden alle Bezeichner bis zum ersten vorgefundenen Block
    /// [ein_block_bezeichner] eingelesen (plus Bezeichner die spaeter in
    /// hinter einem leeren Blockbezeichner [] auftreten, dies sollte aber
    /// aus Lesbarkeitsgruenden nicht verwendet werden)
    ValueParser( const char * fname,
                 const char * block = 0 );
    /** \short reads options from an array of strings.
        every string beginning with a
        -c
        where c is arbitrary charcter not in {0,1,2,3,4,5,6,8,9}
        is treated like an option.
        All following strings which are not recognized as options are concatenated
        to build the value of this option
        \param prefix_str if != 0 this is the option prefix which will be required for
        all options. Take for example a prefix_str= "dum_"
        then
        -dum_a 1
        will be recognized as option a with value 1
    */
    ValueParser( int argc,
                 char const * const * argv,
                 const char * prefix_str = 0 );
    /** \short reads options from an character array.

     */
    ValueParser( int mode,
                 const char * line );

    ~ValueParser() {}


    // Public methods

    /// beim Einlesen von Variablen mittels get(...) werden die einglesenen
    /// Werte immer ausgegeben. Die Voreinstellung ist set_verbose(false);
    void set_verbose( bool b = true )
      {
          verbose_mode = b;
      }

    /// print warnings in suspicious cases
    void set_warnings( bool b = true )
      {
          warnings_mode = b;
      }

    int num_of_not_accessed_entries() const;
    int num_of_accessed_entries() const;
    int show_not_accessed_entries( std::ostream & ) const;
    //

    //input of singleton types

    /// liest einen Eintrag vom Typ "<string>= <int>"
    int get( const char * key,
             int & value ) const
      {
          return getValue( key, type_int, ( void* )&value );
      }

    /// wie vorherige Funktion, aber mit default Wert
    int get( const char * key,
             int & value,
             int def_value ) const
      {
          value = def_value;
          return get( key, value );
      }

    /// liest einen Eintrag vom Typ "<string>= <long>"
    int get( const char * key,
             long & value ) const
      {
          return getValue( key, type_long, ( void* )&value );
      }

    /// wie vorherige Funktion, aber mit default Wert
    int get( const char * key,
             long & value,
             long def_value ) const
      {
          value = def_value;
          return get( key, value );
      }

    /// liest einen Eintrag vom Typ "<string>= <float>"
    int get( const char * key,
             float & value ) const
      {
          return getValue( key, type_float, ( void* )&value );
      }

    /// wie vorherige Funktion, aber mit default Wert
    int get( const char * key,
             float & value,
             float def_value ) const
      {
          value = def_value;
          return get( key, value );
      }

    /// liest einen Eintrag vom Typ "<string>= <double>"
    int get( const char * key,
             double & value ) const
      {
          return getValue( key, type_double, ( void* )&value );
      }

    /// wie vorherige Funktion, aber mit default Wert
    int get( const char * key,
             double & value,
             double def_value ) const
      {
          value = def_value;
          return get( key, value );
      }

#if 0
    /// liest einen Eintrag vom Typ "<string>= <string>"
    int get( const char * key,
             string& value ) const
      {
          return getValue( key, type_string, ( void* )&value );
      }

    /// wie vorherige Funktion, aber mit default Wert
    int get( const char * key,
             string & value,
             const string & def_value ) const
      {
          value = def_value;
          return get( key, value );
      }

#endif

    /// liest einen Eintrag vom Typ "<string>= { "true" | "false" | <int> }
    /// wobei eine Warnung ausgegeben wird, falls <int> nicht aus {0,1} ist.
    /// Werte != 0 werden dabei als "true" aufgefasst
    int get( const char * key,
             bool & value ) const
      {
          return getValue( key, type_bool, ( void* )&value );
      }

    /// wie vorherige Funktion, aber mit default Wert
    int get( const char * key,
             bool & value,
             bool def_value ) const
      {
          value = def_value;
          return get( key, value );
      }

    /// liest einen Eintrag vom Typ "<string>= <char>"
    int get( const char * key,
             char & value ) const
      {
          return getValue( key, type_char, ( void* )&value );
      }

    /// wie vorherige Funktion, aber mit default Wert
    int get( const char * key,
             char & value,
             char def_value ) const
      {
          value = def_value;
          return get( key, value );
      }

    //input of array types

    /// Das Feld "value" muss Speicher fuer mindestens "len" Eintraege reserviert haben.
    /// Das gilt auch, wenn nicht so viele Eintraege in einer Zeile stehen
    /// sollten. Die restlichen Werte werden auf einen Defaultwert gesetzt
    int get( const char * key,
             int * value,
             int len ) const
      {
          return getValue( key, type_int, ( void* )value, len );
      }

    /// wie vorherige Funktion, aber mit default Wert fuer alle Eintraege
    int get( const char * key,
             int* value,
             int len,
             int def_value ) const
      {
          for ( int i = 0; i < len; i++ ) value[i] = def_value;

          return get( key, value, len );
      }

    /// Das Feld "value" muss Speicher fuer mindestens "len" Eintraege reserviert haben.
    /// Das gilt auch, wenn nicht so viele Eintraege in einer Zeile stehen  sollten.
    int get( const char * key,
             long* value,
             int len ) const
      {
          return getValue( key, type_long, ( void* )value, len );
      }

    /// wie vorherige Funktion, aber mit default Wert fuer alle Eintraege
    int get( const char * key,
             long * value,
             int len,
             long def_value ) const
      {
          for ( int i = 0; i < len; i++ ) value[i] = def_value;

          return get( key, value, len );
      }

    /// Das Feld "value" muss Speicher fuer mindestens "len" Eintraege reserviert haben.
    /// Das gilt auch, wenn nicht so viele Eintraege in einer Zeile stehen sollten.
    int get( const char * key,
             float* value,
             int len ) const
      {
          return getValue( key, type_float, ( void* )value, len );
      }

    /// wie vorherige Funktion, aber mit default Wert fuer alle Eintraege
    int get( const char * key,
             float* value,
             int len,
             float def_value ) const
      {
          for ( int i = 0; i < len; i++ ) value[i] = def_value;

          return get( key, value, len );
      }

    /// Das Feld "value" muss Speicher fuer mindestens "len" Eintraege reserviert haben.
    /// Das gilt auch, wenn nicht so viele Eintraege in einer Zeile stehen sollten.
    int get( const char * key,
             double* value,
             int len ) const
      {
          return getValue( key, type_double, ( void* )value, len );
      }

    /// wie vorherige Funktion, aber mit default Wert fuer alle Eintraege
    int get( const char * key,
             double * value,
             int len,
             double def_value ) const
      {
          for ( int i = 0; i < len; i++ ) value[i] = def_value;

          return get( key, value, len );
      }

    /// Das Feld "value" muss Speicher fuer mindestens "len" Eintraege reserviert haben.
    /// Das gilt auch, wenn nicht so viele Eintraege in einer Zeile stehen sollten.
    int get( const char * key,
             bool* value,
             int len ) const
      {
          return getValue( key, type_bool, ( void* )value, len );
      }

    /// wie vorherige Funktion, aber mit default Wert fuer alle Eintraege
    int get( const char * key,
             bool* value,
             int len,
             bool def_value ) const
      {
          for ( int i = 0; i < len; i++ ) value[i] = def_value;

          return get( key, value, len );
      }

    /** Das Feld "value" muss Speicher fuer mindestens "len" Eintraege reserviert haben.
        Das gilt auch, wenn nicht so viele Eintraege in einer Zeile stehen sollten.
        Beachte zusaetzlich, dass bei char arrays (den eigentlichen c strings) das abschliessende
        '\0' Zeichen explizit zum char array gehoert.
        So wird z.B txt = 123 als txt= 123'\0' aufgefasst, und
        get("txt",c_arr,3) eine Warnung ausgibt, dass der eingebeben String laenger ist als vom
        Benutzer angenommen.
        Zu kurze Eingaben werden nicht mit einer Warnung versehen (es sei denn die Eingabe
        hatte laenge 0) <- dies ist nur bei char arrays so.
    */
    int get( const char * key,
             char* value,
             int len ) const
      {
          return getValue( key, type_char, ( void* )value, len );
      }

    /// wie vorherige Funktion, aber mit default Wert fuer alle Eintraege
    int get( const char * key,
             char* value,
             int len,
             char def_value ) const
      {
          for ( int i = 0; i < len; i++ ) value[i] = def_value;

          return get( key, value, len );
      }

    /// wie vorherige Funktion, aber mit default Werten fuer alle Eintraege
    int get( const char * key,
             char * value,
             int len,
             const char * def_values ) const
      {
          char dum = def_values[0];

          for ( int i = 0; i < len; i++ )
          {
              //cout << "\n def_values= " << def_values[i] << " dum= " << dum;
              if ( dum != '\0' )
                  dum = def_values[i];

              value[i] = dum;
          }

          return get( key, value, len );
      }


    //input of bool and string arrays not yet supported

    std::ostream & print( std::ostream & os ) const;
};

inline
std::ostream &
operator<<( std::ostream & os,
            const ValueParser & t )
{
    return t.print( os );
}



int
str_to_int_array( bool & warning,
                  const char * value_str,
                  int * value,
                  int len );

int
str_to_long_array( bool & warning,
                   const char * value_str,
                   long * value,
                   int len );

int
str_to_float_array( bool & warning,
                    const char * value_str,
                    float * value,
                    int len );

int
str_to_double_array( bool & warning,
                     const char * value_str,
                     double * value,
                     int len );

int
str_to_bool_array( bool & warning,
                   const char * value_str,
                   bool * value,
                   int len );

int
str_to_char_array( bool & warning,
                   const char * value_str,
                   char * value,
                   int len );
#endif
