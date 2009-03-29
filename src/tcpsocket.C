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
/* author: Artur Merke */
#include "tcpsocket.h"

#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>

#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cerrno>
#include <fcntl.h>

#ifndef ERROR_STREAM
#define ERROR_STREAM std::cerr
#endif

#ifndef ERROR_OUT
#define ERROR_OUT ERROR_STREAM << "\n\n*** ERROR file=\"" << __FILE__ << "\" line=" << __LINE__
#endif

#ifndef WARNING_STREAM
#define WARNING_STREAM std::cerr
#endif

#ifndef WARNING_OUT
#define WARNING_OUT WARNING_STREAM << "\n\n*** WARNING file=\"" << __FILE__ << "\" line=" << __LINE__
#endif


/******************************************************************************/
bool
TCPsocket::send_msg( const char * buf,
                     int len )
{
    //int res = sendto(fd, buf, len, 0, (struct sockaddr *) & remote_addr, sizeof(remote_addr));
    int res = send( fd, buf, ( unsigned int ) len , 0 );

    if ( res != len )
    {
        WARNING_OUT << "\n res " << res << " len= " << len;
        return false ;
    }

    return true;
}

int
TCPsocket::recv_msg( char * buf,
                     int max_len )
{
    int len = recv( fd, buf, max_len, 0 );

    if ( len < 0 )
    {
        if ( len == -1 && errno == EWOULDBLOCK )
        {
            //cout << "-" << flush;
        }
    }

    return len;
}

bool
TCPsocket::close()
{
    if ( ::close( fd ) < 0 )
    {
        return false;
    }

    return true;
}

/******************************************************************************/
#if 1
void which_error()
{
    switch ( errno ) {

    case EBADF:
        std::cerr << "  sockfd is not a valid descriptor.";
        break;

    case EINVAL:
        std::cerr << " The socket is already bound to  an  address.   This\n"
                  << "may change in the future: see linux/unix/sock.c for\n"
                  << "details.\n";
        break;

    case EACCES:
        std::cerr << " The address is protected, and the user is not\n"
                  << " the super-user.\n";
        break;

    case ENOTSOCK:
        std::cerr << "Argument is a descriptor for a file, not a\n"
                  << "socket.\n"
                  << "\n"
                  << "The following errors are specific to UNIX\n"
                  << "domain (AF_UNIX)\n"
                  << "sockets:\n";
        break;
#if 0

    case EINVAL:
        std::cerr << " The addrlen is wrong, or the socket was not\n"
                  << " in the AF_UNIX family.\n";
        break;
#endif

    case EROFS:
        std::cerr << "  The socket inode would reside on a  read-only\n"
                  << "  file system.\n";
        break;

    case EFAULT:
        std::cerr << " my_addr   points   outside  the  user's\n"
                  << "accessible address space.\n";
        break;

    case ENAMETOOLONG:
        std::cerr << " my_addr is too long.\n";
        break;

    case ENOENT:
        std::cerr << " The file does not exist.\n";
        break;

    case ENOMEM:
        std::cerr << " Insufficient kernel memory was available.\n";
        break;

    case ENOTDIR:
        std::cerr << "A component of the path prefix is not a\n"
                  << "directory.\n";
        break;
#if 0

    case EACCES:
        std::cerr << " Search  permission  is denied on a component\n"
                  << " of the path prefix.\n";
        break;
#endif

    case ELOOP:
        std::cerr << "  Too many symbolic links were encountered in resolving\n"
                  << "  my_addr.\n";

    default:
        std::cerr << " UNKOWN ERROR\n";
    }
}

#endif

bool
TCPutils::bind_socket( int fd,
                       sockaddr_in & addr,
                       int port )
{
    memset( ( char * ) &addr, 0, sizeof( addr ) ) ;
    addr.sin_family           = AF_INET ;
    addr.sin_addr.s_addr      = htonl( INADDR_ANY ) ;
    addr.sin_port             = htons( port ) ;

    int res = bind( fd, ( struct sockaddr * ) & addr, sizeof( addr ) );

    if ( res < 0 )
    {
        ERROR_OUT << "\nbind failure";
        which_error();
        return false ;  /* Can't bind local address */
    }

    return true;
}

bool
TCPutils::get_host( const char * host,
                    int port,
                    sockaddr_in & addr )
{

    struct hostent * host_ent;
    struct in_addr * addr_ptr;

    if ( ( host_ent = ( struct hostent * )gethostbyname( host ) ) == NULL )
    {
        /* Check if a numeric address */
        if ( ( int )inet_addr( host ) == -1 )
        {
            ERROR_OUT << "\ngethostbyname failure";
            return false;
        }
    }
    else
    {
        addr_ptr = ( struct in_addr * )*host_ent->h_addr_list ;
        host = inet_ntoa( *addr_ptr ) ;
    }

    memset( ( char * ) &addr, 0, sizeof( addr ) ) ;

    addr.sin_family		= AF_INET ;
    addr.sin_addr.s_addr	= inet_addr( host ) ;
    addr.sin_port		= htons( port ) ;
    return true;
}

bool
TCPutils::init_client( const char * host,
                       int host_port,
                       TCPsocket & tcpsocket )
{
    tcpsocket.fd = socket( PF_INET, SOCK_STREAM, 0 );

    if ( tcpsocket.fd < 0 )
    {
        ERROR_OUT << "\nsocket failure";
        return false ;       /* Can't open socket. */
    }

#if 0
    int i = 1;

    //don't use timeout for reuse
    setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof( i ) );

#endif

    if ( ! get_host( host, host_port, tcpsocket.remote_addr ) )
    {
        return false;
    }

    int res = connect( tcpsocket.fd,
                       ( struct sockaddr * )&tcpsocket.remote_addr,
                       sizeof( tcpsocket.remote_addr ) );

    if ( res < 0 )
    {
        ERROR_OUT << "\nconnect failure";
        return false;
    }

    return true;
}

bool
 TCPutils::set_fd_nonblock( int fd )
{
    /* doc:
       O_NONBLOCK
       The bit that enables nonblocking mode for the file. If this bit is set, read requests on the file can return
       immediately with a failure status if there is no input immediately available, instead of blocking. Likewise, write
       requests can also return immediately with a failure status if the output can't be written immediately.

       O_NDELAY
       This is a synonym for O_NONBLOCK, provided for compatibility with BSD.

       FASYNC
       if you set the FASYNC status flag on a file descriptor (see section File Status Flags), a SIGIO signal is sent whenever input
       or output becomes possible on that file descriptor. The process or process group to receive the signal can be selected
       by using the F_SETOWN command to the fcntl function. If the file descriptor is a socket, this also selects the recipient of
       SIGURG signals that are delivered when out-of-band data arrives on that socket; see section Out-of-Band Data.
    */

    if ( fcntl( fd , F_SETOWN, getpid() ) < 0 )
    {
        ERROR_OUT << "\nset_fd_nonblock  failure";
        return false;
    }

    int val = fcntl( fd, F_GETFL, 0 ) ;

#if 1
    val |= O_NDELAY ;
#else
    val |= O_NONBLOCK ;
#endif
    //val |= FASYNC;

    if ( fcntl( fd, F_SETFL, val ) < 0 )
    {
        ERROR_OUT << "\nset_fd_nonblock  failure";
        return false;
    }

    return true;
}

bool
TCPutils::set_fd_sigio( int fd )
{
    /* doc:
       O_NONBLOCK
       The bit that enables nonblocking mode for the file. If this bit is set, read requests on the file can return
       immediately with a failure status if there is no input immediately available, instead of blocking. Likewise, write
       requests can also return immediately with a failure status if the output can't be written immediately.

       O_NDELAY
       This is a synonym for O_NONBLOCK, provided for compatibility with BSD.

       FASYNC
       if you set the FASYNC status flag on a file descriptor (see section File Status Flags), a SIGIO signal is sent whenever input
       or output becomes possible on that file descriptor. The process or process group to receive the signal can be selected
       by using the F_SETOWN command to the fcntl function. If the file descriptor is a socket, this also selects the recipient of
       SIGURG signals that are delivered when out-of-band data arrives on that socket; see section Out-of-Band Data.
    */
    if ( fcntl( fd , F_SETOWN, getpid() ) == -1 )
    {
        ERROR_OUT << "\nset_fd_sigio  failure";
        return false;
    }

    int val = fcntl( fd, F_GETFL, 0 ) ;

#ifndef FASYNC
    // I don't know the Solaris equivalent
    // should probably throw exception here.
#else
    val |= FASYNC;
#endif

    if ( fcntl( fd, F_SETFL, val ) < 0 )
    {
        ERROR_OUT << "\nset_fd_sigio failure";
        return false;
    }

    return true;
}

/******************************************************************************/

bool
TCPserver::init( int port )
{
    fd = socket( PF_INET, SOCK_STREAM, 0 );

    if ( fd < 0 )
    {
        ERROR_OUT << "\nsocket failure";
        return false ;       /* Can't open socket. */
    }

#if 1
    int i = 1;

    //don't use timeout for reuse
    setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof( i ) );

#endif

    struct sockaddr_in addr;

    if ( ! TCPutils::bind_socket( fd, addr, port ) )
    {
        return false;
    }

    return true;
}

bool
TCPserver::listen()
{
    int res = ::listen( fd, 1 ); //just accept one connection

    if ( res )
    {
        ERROR_OUT << "\nlisten failure";
        return false;
    }

    return true;
}

bool
TCPserver::accept( TCPsocket & tcpsocket )
{
#ifndef HAVE_SOCKLEN_T
    size_t len;
#else
    socklen_t len;
#endif

    len = sizeof( tcpsocket.remote_addr );
    tcpsocket.fd = ::accept( fd, ( struct sockaddr * ) & tcpsocket.remote_addr, & len );

    if ( tcpsocket.fd < 0 )
    {
        return false;
    }

    return true;
}

bool
 TCPserver::close()
{
    if ( ::close( fd ) < 0 )
    {
        return false;
    }

    return true;
}

/******************************************************************************/



/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/***************  T  E  S  T**************************************************/

#if 0

int
recv_data_with_blocking( TCPsocket & sock )
{
    const int MAXMESG = 10000;
    char buf[MAXMESG];

    while ( 1 )
    {
        int res = sock.recv_msg( buf, MAXMESG );
        cout << "\nrecv_msg       = " << res;

        if ( res <= 0 )
        {
            cout << "\nclosing connection" << flush;
            sock.close();
            break;
        }

        else
            cout << "\n[" << buf << "]" << flush;
    }
}

int
recv_data( TCPsocket & sock )
{
    const int MAXMESG = 10000;
    char buf[MAXMESG];

    fd_set rfds;

    struct timeval tv;
    int retval;

    while ( 1 )
    {
        FD_ZERO( &rfds );
        //FD_SET(server.fd, &rfds);
        FD_SET( sock.fd, &rfds );
        int max_fd_plus_1 =  sock.fd;
        //if (server.fd > max_fd_plus_1) max_fd_plus_1= server.fd;
        max_fd_plus_1++;

        /* Wait up to five seconds. */
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        retval = select( max_fd_plus_1, &rfds, NULL, NULL, &tv );
        /* Don't rely on the value of tv now! */

        //if ( FD_ISSET(server.fd, &rfds) )
        //  cout << "\nSERVER recv_msg";

        if ( FD_ISSET( sock.fd, &rfds ) )
        {
            int res = sock.recv_msg( buf, MAXMESG );
            cout << "\nrecv_msg       = " << res;

            if ( res <= 0 )
            {
                cout << "\nclosing connection" << flush;
                sock.close();
                break;
            }

            else
                cout << "\n[" << buf << "]" << flush;
        }
    }
}

int
main( int argc, char ** argv )
{
    const int MAXMESG = 10000;
    char buf[MAXMESG];

    if ( argc > 1 )   //client
    {
        TCPsocket sock;
        bool res = TCPutils::init_client( "localhost", 20000, sock );

        if ( res )
        {
            //for (int i=0; i<1000; i++)
            //  cout << "\nsend_msg       = " << sock.send_msg("(init BS2k (version 5.25))",20);
            cout << "\nsend_msg       = " << sock.send_msg( "AAAAAAAAAAAAAAAAAAAAAAAAAAA", 20 );
        }

        char ch;

        cin >> ch;
        sock.close();
        return 0;
    }

    TCPserver server;

    cout << "\nserver.init= " << server.init( 20000 ) << flush;
    cout << "\nlisten     = " << server.listen() << flush;

    TCPsocket sub_sock;
#if 0
    //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

    while ( server.accept( sub_sock ) )
    {
        cout << "\n serverfd= " << server.fd << " data from " << sub_sock.fd << flush;
        ////int len = recv(sub_sock.fd, buf, MAXMESG, 0);
#if 0
        int len = sub_sock.recv_msg( buf, MAXMESG );
        cout << "\n got " << len << "bytes= [" << buf << "]" << flush;
#else
        //recv_data(sub_sock);
        recv_data_with_blocking( sub_sock );
#endif
    }

    cout << "\nend" << endl;

    return 0;
    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#endif

#if 1
    fd_set rfds;

    struct timeval tv;
    int retval;

    while ( 1 )
    {
        FD_ZERO( &rfds );
        //FD_SET(0, &rfds); //Standardeingabe
        FD_SET( server.fd, &rfds );
        int max_fd_plus_1 =  server.fd + 1;

        /* Wait up to five seconds. */
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        retval = select( max_fd_plus_1, &rfds, NULL, NULL, &tv );
        /* Don't rely on the value of tv now! */

        if ( retval <= 0 )
        {
            cout << "\nretval= " << retval << flush;
            continue;
        }

        if ( FD_ISSET( server.fd, &rfds ) )
        {
            bool res = server.accept( sub_sock );

            if ( ! res )
            {
                cout << "\nnothing to accept" << flush;
                continue;
            }

            cout << "\n data from " << sub_sock.fd << flush;

            //continue; // <=================== DEBUG
            //TCPutils::set_fd_nonblock(sub_sock.fd); //should be indifferent here
            recv_data( sub_sock );
        }
    }

#endif
}

#endif
