%define name	libvorbis
%define version	1.0.0
%define release 1

Summary:	The Vorbis General Audio Compression Codec
Name:		%{name}
Version:	%{version}
Release:	%{release}
Group:		Libraries/Multimedia
Copyright:	LGPL
URL:		http://www.xiph.org/
Vendor:		Xiphophorus <team@xiph.org>
Source:		ftp://ftp.xiph.org/pub/ogg/vorbis/%{name}-%{version}.tar.gz
BuildRoot:	%{_tmppath}/%{name}-root
Requires:	libogg >= 1.0.0

%description
Ogg Vorbis is a fully open, non-proprietary, patent-and-royalty-free,
general-purpose compressed audio format for audio and music at fixed 
and variable bitrates from 16 to 128 kbps/channel.

%package devel
Summary: 	Vorbis Library Development
Group: 		Development/Libraries
Requires:	libogg-devel >= 1.0.0

%description devel
The libvorbis-devel package contains the header files and documentation
needed to develop applications with libvorbis.

%prep
%setup -q -n %{name}-%{version}

%build
if [ ! -f configure ]; then
  CFLAGS="$RPM_FLAGS" ./autogen.sh --prefix=/usr
else
  CFLAGS="$RPM_FLAGS" ./configure --prefix=/usr
fi
make

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install

%files
%defattr(-,root,root)
%doc COPYING
%doc README
/usr/lib/libvorbis.so*
/usr/lib/libvorbisfile.so*

%files devel
%doc doc/programming.html
%doc doc/v-comment.html
%doc doc/vorbis.html
%doc doc/wait.png
%doc doc/vorbisword2.png
%doc doc/white-ogg.png
%doc doc/white-xifish.png
/usr/include/vorbis/codec.h
/usr/include/vorbis/backends.h
/usr/include/vorbis/codebook.h
/usr/include/vorbis/mode_A.h
/usr/include/vorbis/mode_B.h
/usr/include/vorbis/mode_C.h
/usr/include/vorbis/mode_D.h
/usr/include/vorbis/mode_E.h
/usr/include/vorbis/modes.h
/usr/include/vorbis/vorbisfile.h
/usr/include/vorbis/book/lsp*.vqh
/usr/include/vorbis/book/res0_*.vqh
/usr/include/vorbis/book/resaux0_*.vqh
/usr/lib/libvorbis.a

%clean 
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%changelog
* Sat Oct 21 2000 Jack Moffitt <jack@icecast.org>
- initial spec file created
