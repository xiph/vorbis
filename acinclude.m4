# acinclude.m4
# all .m4 files needed that might not be installed go here

# ogg.m4
# Configure paths for libogg
# Jack Moffitt <jack@icecast.org> 10-21-2000
# Shamelessly stolen from Owen Taylor and Manish Singh

dnl AM_PATH_OGG([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for libogg, and define OGG_CFLAGS and OGG_LIBS
dnl
AC_DEFUN(AM_PATH_OGG,
[dnl 
dnl Get the cflags and libraries from the ogg-config script
dnl
AC_ARG_WITH(ogg-prefix,[  --with-ogg-prefix=PFX   Prefix where libogg is installed (optional)], ogg_prefix="$withval", ogg_prefix="")
AC_ARG_ENABLE(oggtest, [  --disable-oggtest       Do not try to compile and run a test Ogg program],, enable_oggtest=yes)

  if test x$ogg_prefix != x ; then
     ogg_args="$ogg_args --prefix=$ogg_prefix"
     if test x${OGG_CONFIG+set} != xset ; then
        OGG_CONFIG=$ogg_prefix/bin/ogg-config
     fi
  fi

  AC_PATH_PROG(OGG_CONFIG, ogg-config, no)
  min_ogg_version=ifelse([$1], ,1.0.0,$1)
  AC_MSG_CHECKING(for Ogg - version >= $min_ogg_version)
  no_ogg=""
  if test "$OGG_CONFIG" = "no" ; then
    no_esd=yes
  else
    OGG_CFLAGS=`$OGG_CONFIG $oggconf_args --cflags`
    OGG_LIBS=`$OGG_CONFIG $oggconf_args --libs`

    ogg_major_version=`$OGG_CONFIG $ogg_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    ogg_minor_version=`$OGG_CONFIG $ogg_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    ogg_micro_version=`$OGG_CONFIG $ogg_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_oggtest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $OGG_CFLAGS"
      LIBS="$LIBS $OGG_LIBS"
dnl
dnl Now check if the installed Ogg is sufficiently new. (Also sanity
dnl checks the results of ogg-config to some extent
dnl
      rm -f conf.oggtest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>

int main ()
{
  system("touch conf.oggtest");
  return 0;
}

],, no_ogg=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_ogg" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$OGG_CONFIG" = "no" ; then
       echo "*** The ogg-config script installed by Ogg could not be found"
       echo "*** If Ogg was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the OGG_CONFIG environment variable to the"
       echo "*** full path to ogg-config."
     else
       if test -f conf.oggtest ; then
        :
       else
          echo "*** Could not run Ogg test program, checking why..."
          CFLAGS="$CFLAGS $OGG_CFLAGS"
          LIBS="$LIBS $OGG_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include <ogg/ogg.h>
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding Ogg or finding the wrong"
          echo "*** version of Ogg. If it is not finding Ogg, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means Ogg was incorrectly installed"
          echo "*** or that you have moved Ogg since it was installed. In the latter case, you"
          echo "*** may want to edit the ogg-config script: $OGG_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     OGG_CFLAGS=""
     OGG_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(OGG_CFLAGS)
  AC_SUBST(OGG_LIBS)
  rm -f conf.oggtest
])
