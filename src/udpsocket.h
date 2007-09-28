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
#ifndef _UDPSOCKET_H_
#define _UDPSOCKET_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <netinet/in.h>

class UDPsocket {
 public:
  static void set_fd_nonblock(int fd);
  static void set_fd_sigio(int fd);
 public:
  void set_fd_nonblock() { set_fd_nonblock(socket_fd); }
  void set_fd_sigio() { set_fd_sigio(socket_fd); }

    enum {
        MAXMESG= 4096
    };

  int			socket_fd;
  struct sockaddr_in	serv_addr;
  UDPsocket() { socket_fd= -1; }
  bool init_socket_fd(int port= 0);
  bool init_serv_addr(const char* host, int port);
  bool send_msg(const char* buf, int len);
  bool recv_msg(char *buf, int & len, bool redirect= false);
  bool recv_msg(char *buf, int & len, int max_len, bool redirect);
  void close_socket_fd();
};

#endif
