// -*-c++-*-

/*!
  \file label_atlas.h
  \brief the images of the labels of the 3D view. Header File.
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
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifndef RCSSMONITOR_LABEL_ATLAS_H
#define RCSSMONITOR_LABEL_ATLAS_H

#include <QFont>
#include <QImage>
#include <QRectF>
#include <QString>

#include <array>

/*!
  \class LabelAtlas
  \brief one image with the number labels of all the players, side by side
  (an "atlas"), so that one texture serves them all (rcssmonitor-web
  numberSprite(), setNumberText()). Each player has a slot of
  LABEL_WIDTH x LABEL_HEIGHT pixels with its text, white with a black
  outline, centered; one more slot is plain white, for the cards. A slot is
  drawn again only when its text or the font changes. It knows nothing of
  OpenGL: Scene3D sends image() to the GPU when it changed.
*/
class LabelAtlas {
public:

    static constexpr int LABEL_WIDTH = 192; //!< a slot (pixels)
    static constexpr int LABEL_HEIGHT = 64;
    static constexpr int LABEL_FONT = 40; //!< the text's size in a slot (pixels)
    static constexpr double LABEL_SCALE = 1.3; //!< a slot's height in the world (m), before SceneBuilder fits it
    static constexpr int PLAYER_SLOTS = 22; //!< one per player: the left 0-10, the right 11-21
    static constexpr int WHITE_SLOT = PLAYER_SLOTS; //!< plain white, for the cards

    LabelAtlas();

    /*!
      \brief set the text of a slot, drawn with the family and weight of the font.
      \return true if the slot was drawn again
    */
    bool setText( const int slot,
                  const QString & text,
                  const QFont & font );

    //! where a slot is in the image (0 to 1, y down as the image's rows)
    QRectF uv( const int slot ) const;

    //! how wide the text of a slot is in the world at the label's size (m)
    double textWidth( const int slot ) const;

    //! the image of all the slots
    const QImage & image() const { return M_image; }

    //! the image changed since the last call (and is to be sent to the GPU again)
    bool takeChanged();

private:
    static constexpr int COLUMNS = 4;
    static constexpr int ROWS = 6; //!< 24 slots: 22 players, white, one free

    QImage M_image;
    std::array< QString, PLAYER_SLOTS > M_keys; //!< what each slot was drawn with
    std::array< double, PLAYER_SLOTS > M_widths; //!< the widths of the texts (pixels)
    bool M_changed;

    QRect slotRect( const int slot ) const;
};

#endif
