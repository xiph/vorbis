Summary: The OGG Vorbis lossy audio compression codec.
Name:  vorbis
Version: 0.0
Release: 1
Copyright: GPL
Group: Development/Libraries
Source: http://www.xiph.org/vorbis/download/%{name}-%{version}.src.tgz 
Url: http://www.xiph.org/vorbis/index.html
BuildRoot: /var/tmp/vorbis-root

%description 
Ogg Vorbis is a fully Open, non-proprietary, patent-and-royalty-free,
general-purpose compressed audio format for high quality (44.1-48.0kHz,
16+ bit, polyphonic) audio and music at fixed and variable bitrates
from 16 to 128 kbps/channel. This places Vorbis in the same class as
audio representations including MPEG-1 audio layer 3, MPEG-4
audio (AAC and TwinVQ), and PAC.

%package devel
Copyright: LGPL
Summary: Development library for OGG Vorbis
Group: Development/Libraries

%description devel
Ogg Vorbis is a fully Open, non-proprietary, patent-and-royalty-free,
general-purpose compressed audio format for high quality (44.1-48.0kHz,
16+ bit, polyphonic) audio and music at fixed and variable bitrates 
from 16 to 128 kbps/channel. This places Vorbis in the same class as 
audio representations including MPEG-1 audio layer 3, MPEG-4 
audio (AAC and TwinVQ), and PAC.

%prep
%setup -q

%build
rm -rf $RPM_BUILD_ROOT
CFLAGS="${RPM_OPT_FLAGS}" ./configure --prefix=/usr
make  

%install
rm -rf $RPM_BUILD_ROOT

install -d $RPM_BUILD_ROOT/usr/include/vorbis 
install -d $RPM_BUILD_ROOT/usr/include/vorbis/book
install -d $RPM_BUILD_ROOT/usr/lib
install -d $RPM_BUILD_ROOT/usr/bin
install -m 0755 lib/libvorbis.a $RPM_BUILD_ROOT/usr/lib/
install -m 0755 lib/vorbisfile.a $RPM_BUILD_ROOT/usr/lib/
install -m 0644 include/vorbis/*.h $RPM_BUILD_ROOT/usr/include/vorbis/
install -m 0644 include/vorbis/book/*.vqh $RPM_BUILD_ROOT/usr/include/vorbis/book/
install -m 0755 -s huff/{residuesplit,huffbuild} $RPM_BUILD_ROOT/usr/bin
install -m 0755 -s vq/{genericvqtrain,lspvqtrain,residuevqtrain,\
vqbuild,vqcascade,vqmetrics,vqpartition} \
                   $RPM_BUILD_ROOT/usr/bin/

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)

%doc README 
/usr/bin/*

%files devel
%defattr(-,root,root)
%doc README docs/*.{png,html}
/usr/include/vorbis/*
/usr/lib/*

%changelog
* Sat Apr 29 2000 Peter Jones <pjones@redhat.com>
- first pass.
