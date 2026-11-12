// -*-c++-*-

/*!
  \file test_gl_isolation.cpp
  \brief test that only src/view3d/gl calls OpenGL (docs/PLAN_3D-VIEW.md §5.3)
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

#include <QtTest>

#include <QDirIterator>
#include <QFile>
#include <QRegularExpression>

/*
  The 3D view hides OpenGL in the parts in src/view3d/gl (ShaderProgram,
  InstancedMesh, RenderState, GLFrame, ...), and the rest of the code
  uses them (docs/PLAN_3D-VIEW.md §5.3). This test reads the C++ sources
  in src, except src/view3d/gl, without their comments and strings, and
  fails on a call of an OpenGL function (glXxx(...)) or an OpenGL constant
  (GL_XXX). To draw in a new way, add a part (or a function of a part) to
  src/view3d/gl.
*/

namespace {

/*!
  \brief the source without its comments and the contents of its strings
  and characters, with the lines kept (for the line numbers)
*/
QString
codeOnly( const QString & source )
{
    QString out;
    out.reserve( source.size() );
    enum { CODE, LINE_COMMENT, BLOCK_COMMENT, STRING, CHAR } state = CODE;
    for ( int i = 0; i < source.size(); ++i )
    {
        const QChar c = source[i];
        const QChar next = ( i + 1 < source.size() ? source[i + 1] : QChar() );
        switch ( state ) {
        case CODE:
            if ( c == '/' && next == '/' ) { state = LINE_COMMENT; ++i; out += "  "; continue; }
            if ( c == '/' && next == '*' ) { state = BLOCK_COMMENT; ++i; out += "  "; continue; }
            if ( c == '"' ) state = STRING;
            if ( c == '\'' ) state = CHAR;
            out += c;
            break;
        case LINE_COMMENT:
            if ( c == '\n' ) { state = CODE; out += c; } else { out += ' '; }
            break;
        case BLOCK_COMMENT:
            if ( c == '*' && next == '/' ) { state = CODE; ++i; out += "  "; continue; }
            out += ( c == '\n' ? c : QChar( ' ' ) );
            break;
        case STRING:
        case CHAR:
            if ( c == '\\' ) { out += "  "; ++i; continue; }
            if ( ( state == STRING && c == '"' ) || ( state == CHAR && c == '\'' ) ) { state = CODE; out += c; continue; }
            out += ( c == '\n' ? c : QChar( ' ' ) );
            break;
        }
    }
    return out;
}

//! the uses of OpenGL in the code: "file:line: text"
QStringList
openGLUses( const QString & path,
            const QString & code )
{
    static const QRegularExpression use( R"(\b(gl[A-Z]\w*\s*\(|GL_[A-Z0-9_]+))" );
    QStringList found;
    const QStringList lines = code.split( '\n' );
    for ( int i = 0; i < lines.size(); ++i )
    {
        const QRegularExpressionMatch m = use.match( lines[i] );
        if ( m.hasMatch() )
        {
            found << QString( "%1:%2: %3" ).arg( path ).arg( i + 1 ).arg( m.captured( 1 ) );
        }
    }
    return found;
}

}

class TestGLIsolation
    : public QObject {
    Q_OBJECT

private slots:

    void detector()
      {
          // a call and a constant are found; comments, strings and names that only contain "gl" are not
          QCOMPARE( openGLUses( "a", codeOnly( "x;\n  glClear( GL_COLOR_BUFFER_BIT );\n" ) ).size(), 1 );
          QCOMPARE( openGLUses( "a", codeOnly( "gl.glEnable( 1 );" ) ).size(), 1 );
          QCOMPARE( openGLUses( "a", codeOnly( "f( GL_BLEND );" ) ).size(), 1 );
          QCOMPARE( openGLUses( "a", codeOnly( "// glClear( GL_COLOR_BUFFER_BIT )\n/* glEnable() */ \"glEnable()\";" ) ).size(), 0 );
          QCOMPARE( openGLUses( "a", codeOnly( "initializeOpenGLFunctions(); GLuint fbo; QOpenGLWidget w; angle( 1 );" ) ).size(), 0 );
          QCOMPARE( openGLUses( "a", codeOnly( "x;\n\n  glClear( 0 );" ) ), QStringList( "a:3: glClear(" ) );
      }

    void onlyInGL()
      {
          const QString src = QStringLiteral( TEST_SOURCE_DIR "/src" );
          const QString gl_dir = src + "/view3d/gl/";
          QStringList uses;
          int files = 0;
          QDirIterator it( src, QStringList() << "*.cpp" << "*.h", QDir::Files, QDirIterator::Subdirectories );
          while ( it.hasNext() )
          {
              const QString path = it.next();
              if ( path.startsWith( gl_dir ) ) continue;
              QFile file( path );
              QVERIFY( file.open( QIODevice::ReadOnly ) );
              uses << openGLUses( path.mid( src.size() - 3 ), codeOnly( QString::fromUtf8( file.readAll() ) ) );
              ++files;
          }
          QVERIFY( files > 50 ); // the sources were found
          QVERIFY2( uses.isEmpty(), qPrintable( "OpenGL outside src/view3d/gl:\n" + uses.join( '\n' ) ) );
      }
};

QTEST_MAIN( TestGLIsolation )
#include "test_gl_isolation.moc"
