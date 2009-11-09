// -*-c++-*-

/*!
  \file painer_interface.h
  \brief painter interface class Header File.
*/

/*
 *Copyright:

 Copyright (C) The RoboCup Soccer Server Maintenance Group.
 Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.	If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifndef RCSSMONITOR_PAINTER_INTERFADE_H
#define RCSSMONITOR_PAINTER_INTERFADE_H

class QPainter;

class PainterInterface {
protected:

    PainterInterface()
      { }

    virtual
    ~PainterInterface()
      { }

public:

    virtual
    void draw( QPainter & painter ) = 0;

};

#endif
