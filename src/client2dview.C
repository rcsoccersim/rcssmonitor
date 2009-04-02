#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include "udpsocket.h"
#include "tcpsocket.h"
#include "valueparser.h"
//#include "randomize.h"
#include "angle.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SSTREAM
#include <sstream>
#else
#include <strstream>
#endif

#include <unistd.h> // wegen {u}sleep()

void move_frame_5( UDPsocket & sock )
{
#ifdef HAVE_SSTREAM
    std::ostringstream o;
#else
    std::ostrstream o;
#endif

    while ( 1 )
    {
        for ( int t = 1; t <= 50; ++t )
        {
            usleep( 100000 );
            o << "MOV 5 (" <<  20 << "," << t << ");";
#ifdef HAVE_SSTREAM
            sock.send_msg( o.str().c_str(), o.str().length() );
#else
            o << std::ends;
            sock.send_msg( o.str(), std::strlen( o.str() ) );
            o.freeze( false );
#endif
        }
    }
}

void
move_ascii_tree_input( UDPsocket & sock )
{
    //init_drand();
    float angle[10];
    const float a = 0.22;
    const float b = 0.36;
    const float c = 0.46;

    float range[10][2] = { { a, -a},
        {0.78 + b, 0.78 - b}, {b, -b}, { -0.78 + b, -0.78 - b},
        {0.52 + c, 0.52 - c}, { -0.52 + c, -0.52 - c},
        {0.52 + c, 0.52 - c}, { -0.52 + c, -0.52 - c},
        {0.52 + c, 0.52 - c}, { -0.52 + c, -0.52 - c}
    };

    for ( int i = 0; i < 10; i++ )
        angle[i] = 0.5 * ( range[i][0] + range[i][1] );

#ifdef HAVE_SSTREAM
    std::ostringstream o;
#else
    std::ostrstream o;
#endif
    for ( ;; )
    {
        for ( int t = 1; t <= 50; t++ )
        {
            usleep( 100000 );

            for ( int i = 0; i < 10; i++ )
            {
                float max = range[i][0];
                float min = range[i][1];
                float ang;
                //ang= min + (max-min)*float(t)/50.0;
                ang = angle[i];
                float s = drand48();

                if ( s < 0.5 )
                    s = ( max - min ) / 50.0;
                else
                    s = -( max - min ) / 50.0;

                ang += s;

                if ( ang > max )
                    ang = max;
                else if ( ang < min )
                    ang = min;

                o << "MOV " << i + 1 << "(" <<  ang  << ");";

                angle[i] = ang;
            }

#ifdef HAVE_SSTREAM
            sock.send_msg( o.str().c_str(), o.str().length() );
#else
            o << std::ends;
            sock.send_msg( o.str(), std::strlen( o.str() ) );
            o.freeze( false );
#endif
        }
    }
}


void
loop_input( UDPsocket & sock )
{
    for ( ;; )
    {
        for ( int t = 1; t <= 50; t++ )
        {
            usleep( 50000 );
#ifdef HAVE_SSTREAM
            std::ostringstream o;
#else
            std::ostrstream o;
#endif
            o << "MOV 1 (" << 6.28 * float( t ) / 50 << ")" ;
#ifdef HAVE_SSTREAM
            sock.send_msg( o.str().c_str(), o.str().length() );
#else
            o << std::ends;
            sock.send_msg( o.str(), std::strlen( o.str() ) );
            o.freeze( false );
#endif
        }
    }
}

void
file_input( UDPsocket & sock,
            const char * fname )
{
    std::ifstream  in( fname );

    if ( ! in )
    {
        std::cerr << "\nproblems with reading file " << fname;
        exit( 1 );
    }

    char send_buffer[1026];

    send_buffer[0] = '[';
    send_buffer[1] = ']';

    char * buffer = send_buffer + 2;

    while ( in )
    {
        in.getline( buffer, 1023 );
        char *dum = buffer;

        while ( *dum == ' ' )
        {
            dum++;
        }

        if ( *dum == '#' || *dum == '\0' )
        {
            continue;
        }

        std::cout << "\nsending \"" << send_buffer << "\"";

        sock.send_msg( send_buffer, std::strlen( send_buffer ) );
    }
}

void
file_input_over_tcp( const char * fname )
{
    TCPsocket tcp_sock;

    if ( ! TCPutils::init_client( "localhost", 20000, tcp_sock ) )
    {
        //if ( !TCPutils::init_client("robocup7",20000,tcp_sock) )
        return;
    }

    std::ifstream  in( fname );

    if ( ! in )
    {
        std::cerr << "\nproblems with reading file " << fname;
        tcp_sock.close();
        exit( 1 );
    }

    char send_buffer[1026];

#if 0
    send_buffer[0] = '[';
    send_buffer[1] = ']';
    char *buffer = send_buffer + 2;
#else
    char *buffer = send_buffer;
#endif

    while ( in )
    {
        in.getline( buffer, 1023 );
        char * dum = buffer;

        while ( *dum == ' ' )
        {
            ++dum;
        }

        if ( *dum == '#' || *dum == '\0' )
        {
            continue;
        }

        std::cout << "\nsending \"" << send_buffer << "\"";
        tcp_sock.send_msg( send_buffer, std::strlen( send_buffer ) );
    }

    std::cout << "\nTCP SEND END" << std::flush;

    tcp_sock.close();
}

void
command_line_input( UDPsocket & sock )
{
    char buffer[1024];

    while ( 1 )
    {
        std::cout << "\n>";
        std::cin.getline( buffer, 1023 );

        if ( *buffer == 'q' )
        {
            return;
        }

        else
        {
            std::cout << "\nsending \"" << buffer << "\"";
            sock.send_msg( buffer, std::strlen( buffer ) );
        }
    }
}

void
middle_size_input( UDPsocket & )
{

}

#define HALMA
#ifdef HALMA
const int initialGrid[19][19] =
{
    /* ------                    columns                ---------- */
    /* 0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5  6  7  8 */
    /*   0 */  { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*   1 */  { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2, -1, -1, -1, -1, -1},
    /*   2 */  { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2, 2, -1, -1, -1, -1, -1},
    /* r 3 */  { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2, 2, 2, -1, -1, -1, -1, -1},
    /* o 4 */  { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2, 2, 2, 2, -1, -1, -1, -1, -1},
    /* w 5 */  { -1, -1, -1, -1, -1, 0, 0, 0, 0, 2, 2, 2, 2, 2, 0, 0, 0, 0, -1},
    /* s 6 */  { -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1},
    /*   7 */  { -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1},
    /*   8 */  { -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1},
    /*   9 */  { -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1},
    /*   0 */  { -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1},
    /*   1 */  { -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1},
    /*   0 */  { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1},
    /*   3 */  { -1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, -1, -1, -1, -1, -1},
    /*   4 */  { -1, -1, -1, -1, -1, 1, 1, 1, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*   5 */  { -1, -1, -1, -1, -1, 1, 1, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*   6 */  { -1, -1, -1, -1, -1, 1, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*   7 */  { -1, -1, -1, -1, -1, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*   8 */  { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}
};

class HalmaBoard {
    int XCENTER;  // Zentrum des Spielbrettes
    int YCENTER;  //
    int chRadius; //radius of a checker

    static const int BSIZE   = 19;

public:
    HalmaBoard( int size );
    void MatrixRC_2_ScreenXY( int row,
                              int col,
                              double & x,
                              double & y );
};

HalmaBoard::HalmaBoard( int size )
{
    XCENTER = 0;  //300
    YCENTER = 0;  //350
    chRadius = size / 35;
}

void
HalmaBoard::MatrixRC_2_ScreenXY( int row,
                                 int col,
                                 double & x,
                                 double & y )
{
    int LY_offset, LX_offset;
    int bx = col;
    int by = row;

    LX_offset = ( bx - 9 );
    LY_offset = ( by - 9 );

    x = XCENTER + LX_offset * 2 * chRadius + LY_offset * chRadius;
    y = YCENTER - LY_offset * 2 * chRadius;
}

void
halma_input( UDPsocket & sock )
{
    HalmaBoard hb( 100 );

#define c_black "000000"
#define c_red   "ff0000"
#define c_blue  "0000ff"
#define c_green "00ff00"
    char color[7][8] = { c_black, c_red, c_blue , c_green, c_red, c_blue,  c_black};

#ifdef HAVE_SSTREAM
    std::ostringstream o;
#else
    std::ostrstream o;
#endif
    int num_circles = 0;
    //field
    o << "VA (0,0,80,80); EMP 0;";
    o << "\nINS CIRCLE col=" << c_black << " ";

    for ( int row = 0; row < 19; ++row )
    {
        for ( int col = 0; col < 19; ++col )
        {
            double cx, cy;
            hb.MatrixRC_2_ScreenXY( row, col, cx, cy );
            const int idx = initialGrid[row][col];

            if ( idx >= 0 )
            {
                o << "\n  (" << cx << "," << cy << "," << 100.0 / 55.0 << ")";
                num_circles++;
            }
        }
    }

    o << ";";

#if 1

    for ( int row = 0; row < 19; ++row )
    {
        for ( int col = 0; col < 19; ++col )
        {
            double cx, cy;
            hb.MatrixRC_2_ScreenXY( row, col, cx, cy );
            const int idx = initialGrid[row][col];

            if ( idx > 0 )
            {
                int frame = row * 19 + col + 1;
                o << "\nINS FRAME id=" << frame << " (" << cx << "," << cy << ");"
                << "\n  INS " << frame << " CIRCLE fil=1 col=" << color[idx] << " (0,0," << 100.0 / 60.0 << ");";
            }
        }
    }

#endif

#ifdef HAVE_SSTREAM
    sock.send_msg( o.str().c_str(), o.str().length() );
    std::cout << o.str();
    std::cout << "\nsize= " << o.str().length() << ", num_circles= " << num_circles << std::endl;
#else
    o << std::ends;
    sock.send_msg( o.str(), std::strlen( o.str() ) );
    std::cout << o.str();
    std::cout << "\nsize= " << std::strlen( o.str() ) << ", num_circles= " << num_circles << std::endl;
    o.freeze( false );
#endif

    o.seekp( 0 );
    o << "\nINS CIRCLE col=  cccccc";

    for ( int row = 0; row < 19; ++row )
    {
        for ( int col = 0; col < 19; ++col )
        {
            double cx, cy;
            hb.MatrixRC_2_ScreenXY( row, col, cx, cy );
            const int idx = initialGrid[row][col];

            if ( idx < 0 )
            {
                o << "\n  (" << cx << "," << cy << "," << 100.0 / 55.0 << ")";
                num_circles++;
            }
        }
    }

    o << ";" << std::ends;

#ifdef HAVE_SSTREAM
    sock.send_msg( o.str().c_str(), o.str().length() );
#else
    o << std::ends;
    sock.send_msg( o.str(), std::strlen( o.str() ) );
    o.freeze( false );
#endif

}

#endif

#ifdef HAVE_LINUX_JOYSTICK_H
#include "vector2d.h"
#include <cstdlib>
#include <linux/joystick.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

class Robot {
public:
    int frame;

    virtual
    ~Robot()
      { }

    virtual
    void read_joystick( int /*num_axes*/ ,
                        int * /*axis*/ )
      { }

    virtual
    void print_pos( std::ostream & )
      { }

    virtual
    void print_skin( std::ostream & ) = 0;
};

class RobotFreiburg
    : public Robot {
public:
    Vector2d pos;
    Angle ang;
    static const double size = 1.25;

    void print_skin( std::ostream & out )
      {
          out << "INS FRAME id=" << frame << ";"
              << "INS " << frame << " POLYGON fil=1 col=000099 "
              << "(" << size * -0.145 << "," << size * 0.175 << ")"
              << "(" << size *  0.145 << "," << size * 0.175 << ")"
              << "(" << size *  0.160 << "," << size * 0.160 << ")"
              << "(" << size *  0.160 << "," << size * 0.015 << ")"
              << "(" << size *  0.145 << "," << size * 0.0 << ")"
              << "(" << size *  0.145 << "," << size * -0.175 << ")"
              << "(" << size *  -0.145 << "," << size * -0.175 << ")"
              << "(" << size *  -0.145 << "," << size * 0.0 << ")"
              << "(" << size *  -0.160 << "," << size * 0.015 << ")"
              << "(" << size *  -0.160 << "," << size * 0.160 << ");"
              //	<< "(" << size *  -0.145 << "," << size * 0.175 << ")"
              << "INS " << frame << " POLYGON fil=1 col=000000 "
              << "(" << size *  -0.130 << "," << size * -0.005 << ")"
              << "(" << size *   0.130 << "," << size * -0.005 << ")"
              << "(" << size *   0.130 << "," << size *  0.155 << ")"
              << "(" << size *  -0.130 << "," << size *  0.155 << ");"
              //kicker
              << "INS " << frame << " FRAME id=" << frame*10 << " lay=1 (0,0.175);"
              << "INS " << frame*10 << "0 POLYGON fil=1 col=000000 "
              << "(" << size *  -0.125 << "," << size * 0.0 << ")"
              << "(" << size *   0.125 << "," << size * 0.0 << ")"
              << "(" << size *   0.125 << "," << size * 0.020 << ")"
              << "(" << size *  -0.125 << "," << size * 0.020 << ");";
          print_pos( out );
      }

    void print_pos( std::ostream & out )
      {
          out << "MOV " << frame << " (" << pos.x << "," << pos.y << "," << ang.get_value() - M_PI*0.5 << ");";
      }

    void read_joystick( int /*num_axes*/,
                        int * axis )
      {
          const double scale = 0.1;
          Vector2d tmp;
          tmp.init_polar( 1, ang.get_value() );
          pos += scale * double( -axis[1] ) / 32767.0 * tmp;
          ang += scale * double( -axis[3] ) / 32767.0;
      }
};

class RobotGolem
    : public Robot {

    void rotate( std::ostream & out,
                 double r,
                 double angle,
                 int num,
                 Vector2d v0,
                 Vector2d v1,
                 Vector2d v2 = Vector2d( 0, 0 ),
                 Vector2d  v3 = Vector2d( 0, 0 ) )
    {
        if ( num > 0 )
        {
            v0.rotate( angle );
            v0 = r * v0;
            out << "(" << v0.x << "," << v0.y << ")";
        }

        if ( num > 1 )
        {
            v1.rotate( angle );
            v1 = r * v1;
            out << "(" << v1.x << "," << v1.y << ")";
        }

        if ( num > 2 )
        {
            v2.rotate( angle );
            v2 = r * v2;
            out << "(" << v2.x << "," << v2.y << ")";
        }

        if ( num > 3 )
        {
            v3.rotate( angle );
            v3 = r * v3;
            out << "(" << v3.x << "," << v3.y << ")";
        }
    }

public:
    Vector2d pos;
    Angle ang;

    void print_skin( std::ostream & out )
      {
          double factor = 0.2;
          out << "\nINS FRAME id=" << frame << " (" << pos.x << "," << pos.y << "," << ang.get_value() << ");";
          out << "\nINS " << frame << " POLYGON fil=1 col=666666 ";
          Vector2d vec1L( -0.25, -1 ), vec1R( 0.25, -1 );
          rotate( out, factor, 0, 2, vec1L, vec1R );
          rotate( out, factor, M_PI * 2.0 / 3.0, 2, vec1L, vec1R );
          rotate( out, factor, -M_PI * 2.0 / 3.0, 2, vec1L, vec1R );
          Vector2d v0( -0.25, -1.1 ), v1( 0.25, -1.1 ), v2( 0.25, -1.3 ), v3( -0.25, -1.3 );
          out << ";";
          out << "\nINS " << frame << " POLYGON fil=1 col=000000 ";
          rotate( out, factor, 0, 4, v0, v1, v2, v3 );
          out << ";";
          out << "\nINS " << frame << " POLYGON fil=1 col=000000 ";
          rotate( out, factor, M_PI * 2.0 / 3.0, 4, v0, v1, v2, v3 );
          out << ";";
          out << "\nINS " << frame << " POLYGON fil=1 col=000000 ";
          rotate( out, factor, -M_PI * 2.0 / 3.0, 4, v0, v1, v2, v3 );
          out << ";";
      }

    void print_pos( std::ostream & out )
      {
          out << "MOV " << frame << " (" << pos.x << "," << pos.y << "," << ang.get_value() << ");";
      }

    void read_joystick( int /*num_axes*/ ,
                        int * axis )
      {
          const double scale = 0.1;
          pos.x +=  scale * double( axis[0] ) / 32767.0;
          pos.y -=  scale * double( axis[1] ) / 32767.0;
          ang += scale * double( -axis[3] ) / 32767.0;
      }
};

class Joystick {
public:
    enum {
        MAX_NAME_LEN = 512,
    };

    int joystick_fd;
    int num_axes;
    int num_buttons;
    int version;
    char name[MAX_NAME_LEN];
    int * button;
    int * axis;
    bool * button_chg;
    bool * axis_chg;

    Joystick()
      {
          joystick_fd = 0;
          num_axes = 0;
          num_buttons = 0;
          version = 0;
          button = 0;
          axis = 0;
          button_chg = 0;
          axis_chg = 0;
      }

    bool init( const char * joydev )
      {
          if ( ( joystick_fd = open( joydev, O_RDONLY ) ) < 0 )
          {
              std::cerr << "\n*** ERROR file=\"" << __FILE__ << "\" line=" <<__LINE__
                        << "\ncould't open joystic device " << joydev
                        << std::endl;;
              return false;
          }

          fcntl( joystick_fd, F_SETFL, O_NONBLOCK ); //don't block

          unsigned char num;
          ioctl( joystick_fd, JSIOCGVERSION, &version );
          ioctl( joystick_fd, JSIOCGAXES, &num );
          num_axes = num;
          ioctl( joystick_fd, JSIOCGBUTTONS, &num );
          num_buttons = num;
          ioctl( joystick_fd, JSIOCGNAME( MAX_NAME_LEN ), name );
          std::cout << "\nJoystick    = " << name
                    << "\nversion     = " << version
                    << "\nnum_axes    = " << num_axes
                    << "\nnum_buttons = " << num_buttons
                    << std::flush;

          button = new int[num_buttons];
          button_chg = new bool[num_buttons];
          axis = new int[num_axes];
          axis_chg = new bool[num_axes];

          for ( int i = 0; i < num_buttons; i++ )
          {
              button[i] = 0;
              button_chg[i] = 0;
          }

          for ( int i = 0; i < num_axes; i++ )
          {
              axis[i] = 0;
              axis_chg[i] = 0;
          }

          return true;
      }

    int read_all_events()
      {
          int got_data = 0;

          for ( int i = 0; i < num_buttons; ++i )
          {
              button_chg[i] = false;
          }

          for ( int i = 0; i < num_axes; ++i )
          {
              axis_chg[i] = false;
          }

          js_event js;

          while ( 1 )   //read all joystick events
          {
              int size = read( joystick_fd, &js, sizeof( struct js_event ) );

              if ( size != sizeof( struct js_event ) )
              {
                  return got_data;
              }

              ++got_data;

            switch ( js.type & ~JS_EVENT_INIT ) {

            case JS_EVENT_BUTTON:
                button[js.number] = js.value;
                button_chg[js.number] = true;
                break;

            case JS_EVENT_AXIS:
                axis[js.number] = js.value;
                axis_chg[js.number] = true;
                break;
            }
        }
    }
};

bool
joy_input( UDPsocket & sock )
{
#ifdef HAVE_SSTREAM
    std::ostringstream out;
#else
    std::ostrstream out;
#endif

    //init field
    out << "EMP 0; VA (0,0,11,6); BG 009900;"
    //goals
    << "\nINS POLYGON lay=-1 col=ffcc00 fil= 1 (-5.25,-1)(-4.5,-1)(-4.5,1)(-5.25,1);"
    << "\nINS POLYGON lay=-1 col=0000cc fil= 1 ( 5.25,-1)( 4.5,-1)( 4.5,1)( 5.25,1);"
    //field lines and circles
    << "\nINS POLYGON lay=-1 col=ffffff (-4.5,-2.5)(4.5,-2.5)(4.5,2.5)(-4.5,2.5);"
    << "\nINS LINE lay=-1 col=ffffff (0,2.5,0,-2.5) (-4.5,-1.5,-3.5,-1.5) (-3.5,-1.5,-3.5,1.5) (-3.5,1.5,-4.5,1.5) (4.5,-1.5,3.5,-1.5) (3.5,-1.5,3.5,1.5) (3.5,1.5,4.5,1.5);"
    << "\nINS CIRCLE lay=-1 col=ffffff (0,0,1.0);"
    << "\nINS POINT lay=-1 col=ffffff (-2.5,0) (2.5,0);"
    //ball
    << "\nINS FRAME id=1 lay= 1 (1,0,0);"
    << "\nINS 1 CIRCLE col=cc0000 fil= 1 (0,0,0.15);";

    RobotFreiburg r1;
    r1.frame = 2;
    RobotGolem r2;
    r2.pos = Vector2d( 2, 0 );
    r2.frame = 3;

    r1.print_skin( out );
    r2.print_skin( out );

#ifdef HAVE_SSTREAM
    sock.send_msg( out.str().c_str(), out.str().length() );
#else
    out << std::ends;
    sock.send_msg( out.str(), std::strlen( out.str() ) );
    out.freeze( false );
#endif

    //end of field init section
    /////////////////////////////////////////////////////////////////////////////

    Joystick js;

    if ( ! js.init( "/dev/input/js0" ) )
        exit( 1 );

    //end of joystick init section
    /////////////////////////////////////////////////////////////////////////////
    Robot * robot = &r2;

    //  double x= 0;
    while ( 1 )
    {
#if 0
        //x+= 0.041666;
        //x+= 0.02;
        x += 0.01;

        if ( x > 3 )
        {
            x = 0.0;
        }

        char msg[20];
        std::snprintf( msg, 20,
                       "MOV 1 (%lf,%lf); MOV 2 (%lf,2);MOV 3 (3,%lf);",
                       1.2 + x, x, x, -2.5 + x*2 );
        sock.send_msg( msg, std::strlen( msg ) );

        //usleep(41666);
        //usleep(20000);
        usleep( 10000 );

        continue;

#endif
        int num = js.read_all_events();

        std::cout << "\nread " << num << " joystick events";

        if ( num )
        {
            if ( js.button_chg[0] && js.button[0] > 0 )
            {
                robot = &r1;
            }

            if ( js.button_chg[1] && js.button[1] > 0 )
            {
                robot = &r2;
            }

            robot->read_joystick( js.num_axes, js.axis );

            out.seekp( 0 );
            robot->print_pos( out );

#ifdef HAVE_SSTREAM
            sock.send_msg( out.str().c_str(), out.str().length() );
#else
            out << std::ends;
            sock.send_msg( out.str(), std::strlen( out.str() ) );
            out.freeze( false );
#endif
        }

        //usleep(100000);
        usleep( 100000 );
    }

    /////////////////////////////////////////////////////////////////////////////
}

#endif // HAVE_LINUX_JOSTICK_H

int
main ( int  argc, char  *argv[] )
{
    if ( argc <= 1 )
    {
        std::cerr << "\nno arguments specified";
        return 1;
    }
    else
    {
        --argc;
        ++argv;
    }

    bool frame5 = false;

    bool halma = false;
    bool command_line = false;
    bool loop = false;
    bool move_ascii_tree = false;
    bool joy_for_midsize = false;
    char file[255] = "";
    char host[512] = "localhost";
    int  port = 6010;

    bool tcp = false;
    ValueParser vp( argc, argv );

    if ( vp.get( "frame5", frame5 ) == 0 ) frame5 = true;

    if ( vp.get( "halma", halma ) == 0 ) halma = true;

    if ( vp.get( "c", command_line ) == 0 ) command_line = true;

    if ( vp.get( "loop", loop ) == 0 ) loop = true;

    if ( vp.get( "tree", move_ascii_tree ) == 0 ) move_ascii_tree = true;

    if ( vp.get( "joy", joy_for_midsize ) == 0 ) joy_for_midsize = true;

    vp.get( "host", host, 512 );

    vp.get( "port", port );

    vp.get( "file", file, 255 );

    if ( vp.get( "tcp_file", file, 255 ) > 0 )
        tcp = true;

    UDPsocket sock;

    sock.init_socket_fd();

    std::cout << "\nhost= " << host << " port= " << port;

    sock.init_serv_addr( host, port );

    if ( command_line )
    {
        command_line_input( sock );
    }
    else if ( move_ascii_tree )
    {
        move_ascii_tree_input( sock );
    }
    else if ( loop )
    {
        loop_input( sock );
    }
    else if ( halma )
    {
        halma_input( sock );
    }
#ifdef HAVE_LINUX_JOYSTICK_H
    else if ( joy_for_midsize )
    {
        joy_input( sock );
    }
#endif // HAVE_LINUX_JOYSTICK_H
    else if ( frame5 )
    {
        move_frame_5( sock );
    }
    else if ( std::strlen( file ) )
    {
        if ( ! tcp )
        {
            file_input( sock, file );
        }
        else
        {
            file_input_over_tcp( file );
        }
    }

    return 0;
}
