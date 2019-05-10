# Requires %defines of `name', `version' and `release'.
# (`make rpm' takes care of these - you aren't expected to
# use this spec directly)

Name: %{name}
Version: %{version}
Release: %{release}
License: BSD
URL: http://haskell.org/hugs/
Source0: %{name}-%{version}.tar.gz
Summary: Hugs 98 - A Haskell Interpreter

BuildRequires: gcc,make,autoconf
BuildRequires: docbook-utils
BuildRequires: libedit-devel

#for OpenGL/GLUT packages
BuildRequires: libGL-devel
BuildRequires: libGLU-devel
BuildRequires: freeglut-devel

#for OpenAL/ALUT packages
BuildRequires: openal-devel
BuildRequires: freealut-devel

#for X11/HGL packages
BuildRequires: libICE-devel
BuildRequires: libSM-devel
BuildRequires: libX11-devel
BuildRequires: libXi-devel
BuildRequires: libXmu-devel
BuildRequires: libXt-devel
BuildRequires: xorg-x11-proto-devel

%description
Hugs 98 is a functional programming system based on Haskell 98, the de facto
standard for non-strict functional programming languages. Hugs 98 provides an
almost complete implementation of Haskell 98, including:

* Lazy evaluation, higher order functions, and pattern matching.

* A wide range of built-in types, from characters to bignums, and lists to
  functions, with comprehensive facilities for defining new datatypes and type
  synonyms.

* An advanced polymorphic type system with type and constructor class
  overloading.

* All of the features of the Haskell 98 expression and pattern syntax including
  lambda, case, conditional and let expressions, list comprehensions,
  do-notation, operator sections, and wildcard, irrefutable and `as' patterns.

* An implementation of the Haskell 98 primitives for monadic I/O, with support
  for simple interactive programs, access to text files, handle-based I/O, and
  exception handling.

* An almost complete implementation of the Haskell module system. Hugs 98 also
  supports a number of advanced and experimental extensions including
  multi-parameter classes, extensible records, rank-2 polymorphism,
  existentials, scoped type variables, and restricted type synonyms.

%prep
%setup -q 
autoreconf

%build
%configure --enable-threads --enable-char-encoding=locale
make

%install
make DESTDIR=%{buildroot} install_all_but_docs
make -C docs DESTDIR=%{buildroot} install_man

find %{buildroot} -name '*.so' -exec chmod 0755 '{}' ';'

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%doc Credits LICENSE README.md 
%doc docs/ffi-notes.txt
%doc docs/libraries-notes.txt
%doc docs/server.html
%doc docs/users_guide/users_guide
%{_prefix}/bin/hugs
%{_prefix}/bin/runhugs
%{_prefix}/bin/ffihugs
%{_prefix}/bin/cpphs-hugs
%{_prefix}/bin/hsc2hs-hugs
%{_prefix}/bin/happy-hugs
%{_prefix}/bin/hsclour-hugs
%dir %{_libdir}/hugs
%{_libdir}/hugs/*
%{_mandir}/man1/hugs.1.gz
