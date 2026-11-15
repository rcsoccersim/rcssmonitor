// -*-c++-*-

/*!
  \file figure_pack.h
  \brief the figure packs of the 3D view: the models and the motions of the human figures. Header File.
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

#ifndef RCSSMONITOR_FIGURE_PACK_H
#define RCSSMONITOR_FIGURE_PACK_H

#include "figure_looks.h"

#include <QString>
#include <QStringList>

#include <cstdint>
#include <utility>
#include <vector>

/*!
  \struct FigureCredit
  \brief where a model or a motion of a figure pack comes from
*/
struct FigureCredit {
    QString title;
    QString author;
    QString license;
    QString url;

    bool operator==( const FigureCredit & other ) const
      {
          return title == other.title && author == other.author
              && license == other.license && url == other.url;
      }
    //! "<title> by <author> (<license>)", what of them there is
    QString text() const;
};

/*!
  \struct FigureBodySpec
  \brief a body of a figure pack, as its figure.json tells (FigurePack)
*/
struct FigureBodySpec {
    QString model; //!< the model's file (a path of the pack's folder, or of the resources): glTF, GLB, VRM
    QString skeleton; //!< its bones' roles: a preset (skeletonPresets()); empty: found (findHumanBones())
    std::vector< std::pair< QString, QString > > bones; //!< or the pack's own map: a role's name (as VRM writes it) to a bone's name
    FigureSex sex = FigureSex::Any;
    QStringList team; //!< the materials in the player's color (the Mannequin's body)
    QStringList uniform; //!< the materials of the skin, in a uniform of the player's color (FigureUniform)
    QStringList hair; //!< the materials in the player's hair color on all (the eyebrows); a texture's shades over it
    QStringList skin; //!< the materials in the player's skin color; a texture's shades over it
    QStringList tint; //!< the materials in the player's color, a texture's shades over it (clothes of a texture)
    QStringList hairstyles; //!< the materials of the hairstyles: one worn by each player, each as often, in its hair color
    int standard_hairstyle = 0; //!< of hairstyles: worn by a player not known for sure
    struct Extra {
        QString material;
        double chance = 0.0;
    };
    std::vector< Extra > extras; //!< each worn by its chance over a hairstyle, in the hair color (a beard)

    /*!
      \brief the bit of a hairstyle or of an extra, by its material: 1 << its
      index in hairstyles, then in extras; 0 if neither
    */
    std::uint32_t bitOf( const QString & material ) const;
};

/*!
  \struct FigureMotionSpec
  \brief the motion of a figure pack: the model with the clips, and the
  clips of standing, walking, jogging and sprinting (its own, or of the
  pack whose motion it uses)
*/
struct FigureMotionSpec {
    QString pack; //!< whose motion it is: the pack's own name, or another pack's
    QString model; //!< the model with the clips (a path); empty while another pack's is not looked up
    QString skeleton; //!< its bones' roles (as FigureBodySpec::skeleton)
    std::vector< std::pair< QString, QString > > bones; //!< or the pack's own map (as FigureBodySpec::bones)
    QString idle; //!< the clips
    QString walk;
    QString jog;
    QString sprint;
    std::vector< FigureCredit > credits; //!< of the pack whose motion it is, if another pack's
};

/*!
  \struct FigurePack
  \brief a figure pack (docs/PLAN_3D-VIEW.md §5.9, docs/FIGURE_PACKS.md): a
  folder with figure.json, the models and the motion of the human figures.
  The bundled ones (the Mannequin, the Human) are packs as well, in the
  resources. The folder's name is the pack's name in the options
  (player_model_3d).

  figure.json tells the format (FigurePacks::FORMAT), the name shown, the
  credits, the motion (its model and its clips, or the name of the pack
  whose motion it uses), and the bodies: each with its model, the
  skeleton's preset, whose body it is, how its materials are painted, and
  the hairstyles and the extras it can wear. read() checks it and the files
  it names; what only reading the models finds (a missing bone, clip or
  material) Figures3D::setPack() tells.
*/
struct FigurePack {
    QString id; //!< its folder's name, its name in the options
    QString dir; //!< its folder
    bool builtin = false; //!< bundled, in the resources
    int format = 0;
    QString name; //!< shown (the id if figure.json has none)
    QString description;
    std::vector< FigureCredit > credits;
    FigureMotionSpec motion;
    std::vector< FigureBodySpec > bodies;
    QString error; //!< why it cannot be read; empty: it can

    bool ok() const { return error.isEmpty(); }
    //! the name shown, or the id
    QString displayName() const { return name.isEmpty() ? id : name; }
    //! whether it has a man's body and a woman's (the option of the bodies applies)
    bool hasSexes() const;
    //! its credits and its motion's, each once
    std::vector< FigureCredit > allCredits() const;
    //! the credits in short: "<author> (<license>)", each once
    QString creditLine() const;
    //! the credits in full, one a line, with their URLs
    QString creditText() const;
    //! the bodies, as the players' looks are drawn from them
    FigureCast cast() const;

    /*!
      \brief read a pack's figure.json (another pack's motion is named in
      motion.pack, looked up by FigurePacks::scan())
      \param dir its folder
      \param builtin bundled
      \return the pack; its error tells why it cannot be read
    */
    static FigurePack read( const QString & dir,
                            const bool builtin );
};

/*!
  \brief where the figure packs are, and the ones there are: the bundled
  ones (in the resources, :/figures), the ones in a folder of the user's
  choice (the option figure_folder_3d), and the ones in the user's data
  folder (rcssmonitor/figures of each XDG data folder: first
  ~/.local/share). Each pack is a folder with a figure.json; a folder of
  packs may also be a pack itself. The first pack of a name is the one;
  another of the same name cannot be read.

  The packs are looked for once, again when the folder changes or when
  refresh() is called (the dialog does when it shows).
*/
namespace FigurePacks {

//! the format of figure.json this version reads
constexpr int FORMAT = 1;
//! the file of a pack
constexpr const char * FILE_NAME = "figure.json";

//! the folder of the bundled packs (in the resources)
QString builtinFolder();
//! the user's data folders of packs, the first the user's own (~/.local/share/rcssmonitor/figures)
QStringList userFolders();

/*!
  \brief look for the packs (and look up the motions of other packs)
  \param folder a folder of packs of the user's choice (empty: none)
  \return the bundled ones, the folder's, the user's
*/
std::vector< FigurePack > scan( const QString & folder );

//! the packs, looked for once (again when the folder changes, or after refresh())
const std::vector< FigurePack > & packs( const QString & folder );

//! the bundled packs
const std::vector< FigurePack > & builtins();

//! a bundled pack by its name, or null
const FigurePack * builtin( const QString & id );

//! a pack by its name (the first of the name), or null
const FigurePack * find( const QString & id,
                         const QString & folder );

/*!
  \brief look for the packs again the next time (why a pack could not be
  drawn is kept until it is drawn: FrameBuilder3D tries it again)
*/
void refresh();

/*!
  \brief move a pack of the user's to the trash: its folder, with all the
  files in it (it can be restored from there). The packs are looked for
  again the next time (refresh()).
  \param pack the pack (of packs())
  \param error why not, if not null
  \return false if the pack is bundled, if its folder is not a pack's own
  (no figure.json in it; the home folder, a root, or a user's data folder
  of packs), or if the trash refuses it (nothing is deleted then)
*/
bool moveToTrash( const FigurePack & pack,
                  QString * error = nullptr );

//! counts the times the packs were looked for (one changed: the packs may have changed)
int generation();

//! a pack read but not drawn (Figures3D::setPack()): why, for the dialog; empty: it was drawn
void noteError( const QString & id,
                const QString & why );
//! why a pack could not be drawn (noteError()), empty if it was not tried or could be
QString drawError( const QString & id );

}

#endif
