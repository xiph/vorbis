# Configure paths for libvorbis
# Jack Moffitt <jack@icecast.org> 10-21-2000
# Shamelessly stolen from Owen Taylor and Manish Singh

dnl AM_PATH_VORBIS([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for libvorbis, and define VORBIS_CFLAGS and VORBIS_LIBS
dnl
AC_DEFUN(AM_PATH_VORBIS,
[dnl 
dnl Get the cflags and libraries from the vorbis-config script
dnl
AC_ARG_WITH(vorbis-prefix,[  --with-vorbis-prefix=PFX   Prefix where libvorbis is installed (optional)], vorbis_prefix="$withval", vorbis_prefix="")
AC_ARG_ENABLE(vorbistest, [  --disable-vorbistest       Do not try to compile and run a test Vorbis program],, enable_vorbistest=yes)

  if test x$vorbis_prefix != x ; then
     vorbis_args="$vorbis_args --prefix=$vorbis_prefix"
     if test x${VORBIS_CONFIG+set} != xset ; then
        VORBIS_CONFIG=$vorbis_prefix/bin/vorbis-config
     fi
  fi

  AC_PATH_PROG(VORBIS_CONFIG, vorbis-config, no)
  min_vorbis_version=ifelse([$1], ,1.0.0,$1)
  AC_MSG_CHECKING(for Vorbis - version >= $min_vorbis_version)
  no_vorbis=""
  if test "$VORBIS_CONFIG" = "no" ; then
    no_esd=yes
  else
    VORBIS_CFLAGS=`$VORBIS_CONFIG $vorbisconf_args --cflags`
    VORBIS_LIBS=`$VORBIS_CONFIG $vorbisconf_args --libs`

    vorbis_major_version=`$VORBIS_CONFIG $vorbis_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    vorbis_minor_version=`$VORBIS_CONFIG $vorbis_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    vorbis_micro_version=`$VORBIS_CONFIG $vorbis_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_vorbistest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $VORBIS_CFLAGS"
      LIBS="$LIBS $VORBIS_LIBS"
dnl
dnl Now check if the installed Vorbis is sufficiently new. (Also sanity
dnl checks the results of vorbis-config to some extent
dnl
      rm -f conf.vorbistest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vorbis/codec.h>

int main ()
{
  system("touch conf.vorbistest");
  return 0;
}

],, no_vorbis=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_vorbis" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$VORBIS_CONFIG" = "no" ; then
       echo "*** The vorbis-config script installed by Vorbis could not be found"
       echo "*** If Vorbis was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the VORBIS_CONFIG environment variable to the"
       echo "*** full path to vorbis-config."
     else
       if test -f conf.vorbistest ; then
        :
       else
          echo "*** Could not run Vorbis test program, checking why..."
          CFLAGS="$CFLAGS $VORBIS_CFLAGS"
          LIBS="$LIBS $VORBIS_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include <vorbis/codec.h>
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding Vorbis or finding the wrong"
          echo "*** version of Vorbis. If it is not finding Vorbis, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means Vorbis was incorrectly installed"
          echo "*** or that you have moved Vorbis since it was installed. In the latter case, you"
          echo "*** may want to edit the vorbis-config script: $VORBIS_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     VORBIS_CFLAGS=""
     VORBIS_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(VORBIS_CFLAGS)
  AC_SUBST(VORBIS_LIBS)
  rm -f conf.vorbistest
])
