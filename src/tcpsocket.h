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
#ifndef _TCPSOCKET_H_
#define _TCPSOCKET_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <netinet/in.h>

struct TCPsocket {
  int fd;
  struct sockaddr_in remote_addr;
  TCPsocket() { fd= -1; }
  bool send_msg(const char * buf, int len);
  int  recv_msg(char * buf, int max_len);
  bool close();
};

struct TCPutils {
  static bool bind_socket(int fd, sockaddr_in & sockaddr, int port);
  static bool get_host(const char * host, int port, sockaddr_in & addr);
  
  static bool init_client(const char * host, int host_port, TCPsocket & tcpsocket);


  static bool set_fd_nonblock(int fd);
  static bool set_fd_sigio(int fd);
};

struct TCPserver {
  int fd;
  TCPserver() { fd= -1; }
  bool init(int port);
  bool listen();
  bool accept(TCPsocket & tcpsocket);
  bool close();
};

#endif
