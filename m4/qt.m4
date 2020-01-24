
# SYNOPSIS
#
#   AX_QT [--with-qt4-moc=PATH] [--with-qt5-moc=PATH]
#
# DESCRIPTION
#
#   The following shell variable is set to either "yes" or "no":
#
#     have_qt
#
#   Additionally, the following variables are exported:
#
#     QT_CFLAGS
#     QT_CXXFLAGS
#     QT_LDFLAGS
#     QT_LDADD
#     QT_MOC
#

AC_DEFUN([AX_QT],
[
  AC_REQUIRE([AC_PROG_CXX])
  AC_REQUIRE([AC_PATH_X])
  AC_REQUIRE([AC_PATH_XTRA])

  QT_REQUIRED_VERSION=ifelse([$1], ,4.1.0,$1)
  QT_REQUIRED_MODULES=ifelse([$2], ,QtCore,"$2")

  QT_MAJOR_VERSION=`echo $QT_REQUIRED_VERSION | cut -d . -f 1`

  AC_MSG_NOTICE([set QT_REQUIRED_VERSION... $QT_REQUIRED_VERSION])
  AC_MSG_NOTICE([set QT_MAJOR_VERSION... $QT_MAJOR_VERSION])
  AC_MSG_NOTICE([set QT_REQUIRED_MODULES... $QT_REQUIRED_MODULES])



#  if "x$QT_REQUIRED_MODULES" = "x" ; then
#     AC_MSG_ERROR([No Qt modules])
#  fi

  AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
  if test "x$PKG_CONFIG" = "xno"; then
    AC_MSG_ERROR(You have to install pkg-config to compile $PACKAGENAME-$VERSION.)
  fi

  for mod in $QT_REQUIRED_MODULES ; do
    AC_MSG_NOTICE(check $mod >= $QT_REQUIRED_VERSION)
    PKG_CHECK_MODULES(QT,
                      $mod >= $QT_REQUIRED_VERSION,
                      have_qt="yes",
                      have_qt="no")
    if test "$have_qt" = "no"; then
      AC_MSG_ERROR(
        [The $mod library >= [$QT_REQUIRED_VERSION] could not be found.])
    fi
  done

  QT_CFLAGS=$($PKG_CONFIG --cflags $QT_REQUIRED_MODULES)
  QT_CXXFLAGS="$QT_CFLAGS"
  QT_CPPFLAGS=""
  QT_LDFLAGS=$($PKG_CONFIG --static --libs-only-L $QT_REQUIRED_MODULES)
  QT_LDADD="$($PKG_CONFIG --static --libs-only-other $QT_REQUIRED_MODULES) $($PKG_CONFIG --static --libs-only-l $QT_REQUIRED_MODULES)"
  AC_MSG_NOTICE([set QT_CXXFLAGS... $QT_CXXFLAGS])
  AC_MSG_NOTICE([set QT_LDFLAGS... $QT_LDFLAGS])
  AC_MSG_NOTICE([set QT_LDADD... $QT_LDADD])
  AC_SUBST(QT_CFLAGS)
  AC_SUBST(QT_CXXFLAGS)
  AC_SUBST(QT_CPPFLAGS)
  AC_SUBST(QT_LDFLAGS)
  AC_SUBST(QT_LDADD)

  # check the path to moc
  QT_MOC=""

  AC_ARG_WITH([qt-moc],
    [  --with-qt-moc=PATH     Qt's moc utilitiy to be used (optional)])
  if test x$with_qt_moc != x && test -x "$with_qt_moc" ; then
    QT_MOC="$with_qt_moc"
  fi

#   AC_CHECK_PROG(QT_MOC, [moc], [moc])

  if test x$QT_MOC = x ; then
    moc_bin=moc
	if test $QT_MAJOR_VERSION -eq 4 ; then
	  moc_bin=moc-qt4
	fi

    for qt_path_tmp in /opt/local /usr/local /usr ; do
      if test -x "${qt_path_tmp}/bin/${moc_bin}" ; then
        QT_MOC="${qt_path_tmp}/bin/${moc_bin}"
        break;
      fi
    done
  fi
  if test x$QT_MOC = x ; then
    have_qt=no
    AC_MSG_ERROR([You must specify the path to Qt's moc command.])
  fi

  AC_MSG_NOTICE([set QT_MOC... $QT_MOC])
  AC_SUBST(QT_MOC)
])
