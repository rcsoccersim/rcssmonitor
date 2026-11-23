// -*-c++-*-

/*!
  \file http_test_server.h
  \brief a small HTTP server on the local machine for the tests of the
  downloads of game logs, so that they never use the network outside
*/

/*
 Copyright (C) The RoboCup Soccer Server Maintenance Group.
 Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/////////////////////////////////////////////////////////////////////

#ifndef RCSSMONITOR_TEST_HTTP_TEST_SERVER_H
#define RCSSMONITOR_TEST_HTTP_TEST_SERVER_H

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QPair>
#include <QPointer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QUrl>

#include <memory>

/*!
  \class HttpTestServer
  \brief answers GET requests on 127.0.0.1 by the responses set for their
  paths (404 for the others), and counts them. One request a connection
  (Connection: close).
*/
class HttpTestServer {
public:

    struct Response {
        int status = 200;
        QByteArray reason = "OK";
        QList< QPair< QByteArray, QByteArray > > headers;
        QByteArray body;
        //! send Content-Length (otherwise the body ends when the connection closes)
        bool content_length = true;
        //! >= 0: the first half of the body, then the rest after this (ms)
        int split_delay = -1;
        //! the first half of the body, and nothing more (the connection kept)
        bool hang = false;
    };

private:

    QTcpServer M_server;
    QHash< QByteArray, Response > M_routes;
    QHash< QByteArray, int > M_requests;
    QByteArray M_user_agent; //!< of the last request

public:

    HttpTestServer()
      {
          M_server.listen( QHostAddress::LocalHost, 0 );
          QObject::connect( &M_server, &QTcpServer::newConnection,
                            &M_server, [this]() {
                                while ( QTcpSocket * socket = M_server.nextPendingConnection() )
                                {
                                    accept( socket );
                                }
                            } );
      }

    bool isListening() const { return M_server.isListening(); }

    //! the URL of a path on this server
    QUrl url( const QByteArray & path ) const
      {
          return QUrl( QStringLiteral( "http://127.0.0.1:%1" ).arg( M_server.serverPort() )
                       + QString::fromLatin1( path ) );
      }

    void route( const QByteArray & path,
                const Response & response )
      {
          M_routes[path] = response;
      }

    //! a 200 response with a body
    static Response ok( const QByteArray & body )
      {
          Response r;
          r.body = body;
          return r;
      }

    //! a redirect to another path of this server
    static Response redirect( const QByteArray & location )
      {
          Response r;
          r.status = 302;
          r.reason = "Found";
          r.headers.append( qMakePair( QByteArray( "Location" ), location ) );
          return r;
      }

    int requests( const QByteArray & path ) const { return M_requests.value( path ); }
    const QByteArray & userAgent() const { return M_user_agent; }

private:

    void accept( QTcpSocket * socket )
      {
          QObject::connect( socket, &QTcpSocket::disconnected,
                            socket, &QObject::deleteLater );
          std::shared_ptr< QByteArray > request( new QByteArray() );
          QObject::connect( socket, &QTcpSocket::readyRead,
                            socket, [this, socket, request]() {
                                request->append( socket->readAll() );
                                const int end = request->indexOf( "\r\n\r\n" );
                                if ( end < 0 ) return;
                                QObject::disconnect( socket, &QTcpSocket::readyRead, nullptr, nullptr );

                                const QList< QByteArray > lines = request->left( end ).split( '\n' );
                                const QList< QByteArray > first = lines.value( 0 ).trimmed().split( ' ' );
                                const QByteArray path = first.value( 1 );
                                for ( const QByteArray & line : lines )
                                {
                                    if ( line.toLower().startsWith( "user-agent:" ) )
                                    {
                                        M_user_agent = line.mid( 11 ).trimmed();
                                    }
                                }
                                ++M_requests[path];
                                respond( socket, path );
                            } );
      }

    void respond( QTcpSocket * socket,
                  const QByteArray & path )
      {
          Response r;
          if ( M_routes.contains( path ) )
          {
              r = M_routes.value( path );
          }
          else
          {
              r.status = 404;
              r.reason = "Not Found";
              r.body = "not found";
          }

          QByteArray head = "HTTP/1.1 " + QByteArray::number( r.status ) + ' ' + r.reason + "\r\n";
          for ( const auto & h : r.headers )
          {
              head += h.first + ": " + h.second + "\r\n";
          }
          if ( r.content_length )
          {
              head += "Content-Length: " + QByteArray::number( r.body.size() ) + "\r\n";
          }
          head += "Connection: close\r\n\r\n";
          socket->write( head );

          if ( r.hang || r.split_delay >= 0 )
          {
              const int half = r.body.size() / 2;
              socket->write( r.body.left( half ) );
              if ( r.hang ) return;
              const QByteArray rest = r.body.mid( half );
              QPointer< QTcpSocket > s( socket );
              QTimer::singleShot( r.split_delay, socket, [s, rest]() {
                  if ( ! s ) return;
                  s->write( rest );
                  s->disconnectFromHost();
              } );
              return;
          }

          socket->write( r.body );
          socket->disconnectFromHost();
      }
};

#endif
