%define name	libvorbis
%define version	1.0beta4
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
Requires:	libogg >= 1.0beta4

%description
Ogg Vorbis is a fully open, non-proprietary, patent-and-royalty-free,
general-purpose compressed audio format for audio and music at fixed 
and variable bitrates from 16 to 128 kbps/channel.

%package devel
Summary: 	Vorbis Library Development
Group: 		Development/Libraries
Requires:	libogg-devel >= 1.0beta4
Requires:	libvorbis-devel = %{version}

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
%doc AUTHORS
%doc README
/usr/lib/libvorbis.so.*
/usr/lib/libvorbisfile.so.*
/usr/lib/libvorbisenc.so.*

%files devel
%doc doc/*.html
%doc doc/*.png
%doc doc/*.txt
%doc doc/vorbisfile
%doc doc/vorbisenc
/usr/share/aclocal/vorbis.m4
/usr/include/vorbis/codec.h
/usr/include/vorbis/vorbisfile.h
/usr/include/vorbis/vorbisenc.h
/usr/lib/libvorbis.a
/usr/lib/libvorbis.so
/usr/lib/libvorbisfile.a
/usr/lib/libvorbisfile.so
/usr/lib/libvorbisenc.a
/usr/lib/libvorbisenc.so

%clean 
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%changelog
* Sat Oct 21 2000 Jack Moffitt <jack@icecast.org>
- initial spec file created
