# Requires %defines of `name', `version' and `release'.
# (`make rpm' takes care of these - you aren't expected to
# use this spec directly)

Summary:      Hugs - A Haskell Interpreter
Vendor:       Galois Connections, Inc.
Name:         %{name}
Version:      %{version}
Release:      %{release}
License:      BSDish
Group:        Development/Languages/Haskell
Packager:     jeff@galois.com
URL:          http://www.haskell.org/hugs
Source0:      %{name}-%{version}.tar.gz
BuildRoot:    %{_tmppath}/%{name}-buildroot
Prefix:       %{_prefix}
Provides:     haskell
Requires:     readline

%description
Hugs 98 is an interpreter for Haskell, a lazy functional programming language.

%prep
%setup -n %{name}-%{version}

%build
cd src/unix
./configure --prefix=%{prefix} --mandir=%{_mandir} --with-fptools=../../fptools --enable-ffi
cd ..
make

%install
cd src
make prefix=$RPM_BUILD_ROOT%{prefix} HUGSDIR=$RPM_BUILD_ROOT%{prefix}/lib/hugs install

%files
%defattr(-,root,root)
%doc License
%doc Readme
%doc Credits
%doc docs/server.html
%doc docs/ffi-notes.txt
%doc docs/winhugs-notes.txt
%doc docs/machugs-notes.txt
%{_mandir}/man1/hugs.1.gz
%{prefix}/bin/hugs
%{prefix}/bin/runhugs
%{prefix}/bin/ffihugs
%{prefix}/lib/hugs/demos
%{prefix}/lib/hugs/include
%{prefix}/lib/hugs/libraries
%{prefix}/lib/hugs/oldlib
