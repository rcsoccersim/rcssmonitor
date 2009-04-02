// -*-c++-*-

/***************************************************************************
                                  rectangle.h
                             Class for rectangle
                             -------------------
    begin                : 2009-04-02
    copyright            : (C) 2009 by The RoboCup Soccer Server
                           Maintenance Group.
    email                : sserver-admin@lists.sourceforge.net
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU GPL as published by the Free Software  *
 *   Foundation; either version 3 of the License, or (at your option) any  *
 *   later version.                                                        *
 *                                                                         *
 ***************************************************************************/

#ifndef RCSSMONITOR_RECTANGLE_H
#define RCSSMONITOR_RECTANGLE_H

class Rectangle {
public:
    bool active;
    int x, y;
    unsigned int width, height;

private:
    bool use_ratio;
    bool use_center;
    int win_width, win_height;
    int p1x,p1y,p2x,p2y;

public:
    Rectangle();

    Rectangle( int window_width,
               int window_height );

    void set_ratio( int window_width,
                    int widow_height );

    void set_origin( int px,
                     int py )
      {
          p1x = px;
          p1y = py;
          p2x = p1x;
          p2y = p1y;
          conform();
      }

    void set_point( int px,
                    int py )
      {
          p2x = px;
          p2y = py;
          conform();
      }
private:
    void conform();
};

#endif
