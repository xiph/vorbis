%define name	libvorbis
%define version	1.0rc3
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
Requires:	libogg >= 1.0rc3
Prefix:		%{_prefix}

%description
Ogg Vorbis is a fully open, non-proprietary, patent-and-royalty-free,
general-purpose compressed audio format for audio and music at fixed 
and variable bitrates from 16 to 128 kbps/channel.

%package devel
Summary: 	Vorbis Library Development
Group: 		Development/Libraries
Requires:	libogg-devel >= 1.0rc3
Requires:	libvorbis-devel = %{version}

%description devel
The libvorbis-devel package contains the header files and documentation
needed to develop applications with libvorbis.

%prep
%setup -q -n %{name}-%{version}

%build
if [ ! -f configure ]; then
  CFLAGS="$RPM_OPT_FLAGS" ./autogen.sh --prefix=%{_prefix}
else
  CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=%{_prefix}
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
%{_libdir}/libvorbis.so.*
%{_libdir}/libvorbisfile.so.*
%{_libdir}/libvorbisenc.so.*

%files devel
%doc doc/*.html
%doc doc/*.png
%doc doc/*.txt
%doc doc/vorbisfile
%doc doc/vorbisenc
%{_datadir}/aclocal/vorbis.m4
%{_includedir}/vorbis/codec.h
%{_includedir}/vorbis/vorbisfile.h
%{_includedir}/vorbis/vorbisenc.h
%{_libdir}/libvorbis.a
%{_libdir}/libvorbis.so
%{_libdir}/libvorbisfile.a
%{_libdir}/libvorbisfile.so
%{_libdir}/libvorbisenc.a
%{_libdir}/libvorbisenc.so

%clean 
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%changelog
* Sun Dec 31 2001 Jack Moffitt <jack@xiph.org>
- Updated for rc3 release.

* Sun Oct 07 2001 Jack Moffitt <jack@xiph.org>
- Updated for configurable prefixes

* Sat Oct 21 2000 Jack Moffitt <jack@icecast.org>
- initial spec file created
