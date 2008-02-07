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
#ifndef _ASCII_DEV_H_
#define _ASCII_DEV_H_

#include "input_dev.h"
#include "udpsocket.h"
#include "tcpsocket.h"
#include "rgbcolor.h"
#include "object2d.h"
#include "multi.h"
#include "valueparser.h"

class AsciiDevice: public InputDevice {
    UDPsocket udp_sock;
    TCPserver tcp_serv;
    TCPsocket tcp_sock;
    bool tcp_serv_can_accept;
    bool tcp_sock_open;

    enum {
        BUFFER_MAX_SIZE= 8192
    };
    char state_string[120];

    static const bool use_udp;
    bool use_tcp;
    static const bool use_stdin;

    struct Options {
        Options() { udp_port= 6010; tcp_port= 20000; }
        int udp_port;
        int tcp_port;
    };

    Options options;

    bool process_options(const ValueParser & vp);

    template <class T>
    void print_option_entry(std::ostream & o,int mode, const char * option, const T & val, const char * description) const;
    void generic_description_of_options(std::ostream & o, int mode) const;

    bool process_tcp_input(fd_set *set, BuilderBase * buid);
public:
    AsciiDevice(int port= 0);
    bool process_options(const char * fname);
    bool process_options(int argc, char const* const* argv);
    //bool process_char_command(BuilderBase * build, char c,double x, double y) { return false; }
    void help_options(std::ostream & o) const;
    void help_char_command(std::ostream & o) const { o << "\nascii commands:\n\n<no commands>"; }
    void generate_file_options(std::ostream & o) const;

    bool init_frames(BuilderBase * build);
    bool init_connection();

    bool destruct();

    bool process_input(fd_set * , BuilderBase * build);

    const char * status_line() const { return state_string; }
    int set_fds(fd_set * set);// { add_fd_to_set(sock.socket_fd,set); return sock.socket_fd; }
    bool got_fds(fd_set * set);// { return is_fd_in_set(sock.socket_fd,set); }
};

template <class T>
void AsciiDevice::print_option_entry(std::ostream & o,int mode, const char * option, const T & val, const char * description) const {
    if (mode == 0)
        o << "\n"
          << "\n# " << description << " (default " << val << ")"
          << "\n" << option << " = "; //<< val;
    else
        o << "\n"
          << "\n" << description
          << "\n-a_" << option << " [" << val << "]";
}

#endif
