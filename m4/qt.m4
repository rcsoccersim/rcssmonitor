# SYNOPSIS
#
#   AX_QT3 [--with-qt3-moc=PATH]
#
# DESCRIPTION
#
#   The following shell variable is set to either "yes" or "no":
#
#     have_qt3
#
#   Additionally, the following variables are exported:
#
#     QT3_CFLAGS
#     QT3_CXXFLAGS
#     QT3_LDFLAGS
#     QT3_LDADD
#     QT3_MOC

# SYNOPSIS
#
#   AX_QT4 [--with-qt4-moc=PATH]
#
# DESCRIPTION
#
#   The following shell variable is set to either "yes" or "no":
#
#     have_qt4
#
#   Additionally, the following variables are exported:
#
#     QT4_CFLAGS
#     QT4_CXXFLAGS
#     QT4_LDFLAGS
#     QT4_LDADD
#     QT4_MOC
#

AC_DEFUN([AX_QT3],
[
  AC_REQUIRE([AC_PROG_CXX])

  AC_MSG_CHECKING(for Qt3)

  QT3_REQUIRED_VERSION=ifelse([$1], ,3.3.0,$1)
  have_qt3="no"

  AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
  if test "x$PKG_CONFIG" = "xno"; then
    AC_MSG_ERROR(You have to install pkg-config to compile $PACKAGENAME-$VERSION.)
  fi

  PKG_CHECK_MODULES(Qt3,
                    qt-mt >= $QT3_REQUIRED_VERSION,
                    have_qt3="yes",
                    have_qt3="no")
  if test "$have_qt3" = "no"; then
    PKG_CHECK_MODULES(Qt3,
                      qt >= $QT3_REQUIRED_VERSION,
                      have_qt3="yes",
                      have_qt3="no")
  fi

  if test "$have_qt3" = "no"; then
    AC_MSG_ERROR([The Qt library >= [$QT3_REQUIRED_VERSION] could not be found.])
  fi

  QT3_CFLAGS=$($PKG_CONFIG --cflags qt-mt)
  QT3_CXXFLAGS="$QT3_CFLAGS"
  QT3_CPPFLAGS=""
  QT3_LDFLAGS=$($PKG_CONFIG --libs-only-L qt-mt)
  QT3_LDADD="$($PKG_CONFIG --libs-only-other qt-mt)$($PKG_CONFIG --libs-only-l qt-mt)"
  AC_MSG_NOTICE([set QT3_CXXFLAGS... $QT3_CXXFLAGS])
  AC_MSG_NOTICE([set QT3_LDFLAGS... $QT3_LDFLAGS])
  AC_MSG_NOTICE([set QT3_LDADD... $QT3_LDADD])
  AC_SUBST(QT3_CFLAGS)
  AC_SUBST(QT3_CXXFLAGS)
  AC_SUBST(QT3_CPPFLAGS)
  AC_SUBST(QT3_LDFLAGS)
  AC_SUBST(QT3_LDADD)

  # check the path to moc
  QT3_MOC=""

  AC_ARG_WITH([qt3-moc],
    [  --with-qt3-moc=PATH     Qt3's moc utilitiy to be used (optional)])
  if test x$with_qt3_moc != x && test -x "$with_qt3_moc" ; then
    QT3_MOC="$with_qt3_moc"
  fi
  if test x$QT3_MOC = x ; then
    for qt3_path_tmp in /usr/qt/3 /usr/lib/qt3 /usr/share/qt3 /usr/lib/qt-3.3 /opt/local/lib/qt3 /opt/local /usr/local /usr ; do
      if test -x "${qt3_path_tmp}/bin/moc" ; then
        QT3_MOC="${qt3_path_tmp}/bin/moc"
        break;
      fi
    done
  fi
  if test x$QT3_MOC = x ; then
    have_qt3=no
    AC_MSG_ERROR([You must specify the path to Qt3's moc command.])
  fi

  AC_MSG_NOTICE([set QT3_MOC... $QT3_MOC])
  AC_SUBST(QT3_MOC)
])



AC_DEFUN([AX_QT4],
[
  AC_REQUIRE([AC_PROG_CXX])

  QT4_REQUIRED_VERSION=ifelse([$1], ,4.1.0,$1)
  QT4_REQUIRED_MODULES=ifelse([$2], ,QtCore,"$2")

  AC_MSG_NOTICE([set QT4_REQUIRED_VERSION... $QT4_REQUIRED_VERSION])
  AC_MSG_NOTICE([set QT4_REQUIRED_MODULES... $QT4_REQUIRED_MODULES])

#  if "x$QT4_REQUIRED_MODULES" = "x" ; then
#     AC_MSG_ERROR([No Qt4 modules])
#  fi

  AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
  if test "x$PKG_CONFIG" = "xno"; then
    AC_MSG_ERROR(You have to install pkg-config to compile $PACKAGENAME-$VERSION.)
  fi

  for mod in $QT4_REQUIRED_MODULES ; do
    AC_MSG_NOTICE(check $mod >= $QT4_REQUIRED_VERSION)
    PKG_CHECK_MODULES(Qt4,
                      $mod >= $QT4_REQUIRED_VERSION,
                      have_qt4="yes",
                      have_qt4="no")
    if test "$have_qt4" = "no"; then
      AC_MSG_ERROR(
        [The $mod library >= [$QT4_REQUIRED_VERSION] could not be found.])
    fi
  done

  QT4_CFLAGS=$($PKG_CONFIG --cflags $QT4_REQUIRED_MODULES)
  QT4_CXXFLAGS="$QT4_CFLAGS"
  QT4_CPPFLAGS=""
  QT4_LDFLAGS=$($PKG_CONFIG --static --libs-only-L $QT4_REQUIRED_MODULES)
  QT4_LDADD="$($PKG_CONFIG --static --libs-only-other $QT4_REQUIRED_MODULES)$($PKG_CONFIG --static --libs-only-l $QT4_REQUIRED_MODULES)"
  AC_MSG_NOTICE([set QT4_CXXFLAGS... $QT4_CXXFLAGS])
  AC_MSG_NOTICE([set QT4_LDFLAGS... $QT4_LDFLAGS])
  AC_MSG_NOTICE([set QT4_LDADD... $QT4_LDADD])
  AC_SUBST(QT4_CFLAGS)
  AC_SUBST(QT4_CXXFLAGS)
  AC_SUBST(QT4_CPPFLAGS)
  AC_SUBST(QT4_LDFLAGS)
  AC_SUBST(QT4_LDADD)

  # check the path to moc
  QT4_MOC=""

  AC_ARG_WITH([qt4-moc],
    [  --with-qt4-moc=PATH     Qt4's moc utilitiy to be used (optional)])
  if test x$with_qt4_moc != x && test -x "$with_qt4_moc" ; then
    QT4_MOC="$with_qt4_moc"
  fi
##    AC_CHECK_PROG(QT4_MOC, [moc], [moc])
  if test x$QT4_MOC = x ; then
      if test -x "/usr/local/bin/moc-qt4" ; then
        QT4_MOC="/usr/local/bin/moc-qt4"
      fi
  fi
  if test x$QT4_MOC = x ; then
    for qt4_path_tmp in /usr/qt/4 /usr/lib/qt4 /usr/share/qt4 /opt/local/libexec/qt4-mac /opt/local /usr/local /usr ; do
      if test -x "${qt4_path_tmp}/bin/moc" ; then
        QT4_MOC="${qt4_path_tmp}/bin/moc"
        break;
      fi
    done
  fi
  if test x$QT4_MOC = x ; then
    have_qt4=no
    AC_MSG_ERROR([You must specify the path to Qt4's moc command.])
  fi

  AC_MSG_NOTICE([set QT4_MOC... $QT4_MOC])
  AC_SUBST(QT4_MOC)
])
