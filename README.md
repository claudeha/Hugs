# [Hugs](https://www.haskell.org/hugs/), a Haskell implementation

```text
------------------------------------------------------------------------------
__   __ __  __  ____   ___      _________________________________________
||   || ||  || ||  || ||__      Hugs 98: Based on the Haskell 98 standard
||___|| ||__|| ||__||  __||     Copyright (c) 1994-2019
||---||         ___||           World Wide Web: http://haskell.org/hugs
||   ||                         Bugs: https://github.com/cjacker/Hugs
||   || Version: 20190509       _________________________________________

------------------------------------------------------------------------------
```

**Hugs (Haskell User´s Gofer System)** was a popular implementation of the Haskell programming language, and had
been used in many books, tutorials as a teaching platform.

The last upstream release was 'Sep2006', and Maintainence was stopped in 2009 .

However, it still works quickly and correctly, can be built and installed in few minutes, without requirement to admin privileges, and has less complicated error messages than ghc. 

Especially, the installation size is very tiny comparing to the tremendous ghc.

Here is a fork of the most recent codes with some improvement.

## Improvement comparing to 'Sep2006':
* Implement **Foreigh Import Wrapper for X86_64**, (OpenGL/GLUT works finally)
* Update haskell base to a decent version: **base-4.0**
* Update essential utilities, include cabal, cpphs, hsc2hs.
* Update various packages
* Bundle **'happy'** parser generator
* Bugfixes
* ...

## Build from release tarball:
The release tarball already includes the source codes of haskell base and other packages.

Download: https://github.com/cjacker/Hugs/releases/download/goodstart/Hugs-May2019.tar.xz

```console
$ autoreconf
$ ./configure 
$ make && make install
```

## Build from source:
```console
## Build
$ git clone https://github.com/cjacker/Hugs
$ cd Hugs
$ ./download-packages.sh
$ autoreconf
$ ./configure 
$ make


## Install
$ make install_all_but_docs
$ make install_man -C docs
```
## Note of building with clang
```console
## Building hugs with clang, need to set env as below:
$ export CC=clang
$ export CXX=clang++
$ export cpp="clang -E -Wno-invalid-pp-token -traditional-cpp -Wno-unicode -Wno-trigraphs"
```

## Requirements:
* gcc/g++/make/autoconf/perl
* libedit/ncurses with dev package
* darcs (only needed when build from git, to fetch base-4.0 source code from haskell.org)
* libsigsegv with dev package (optional)
* X11 with dev packages (optional, to build hackage: X11)
* OpenAL/FreeALUT with dev package (optional, to build hackages: OpenAL/ALUT)
* OpenGL/FreeGLUT with dev package (optional, to build hackages: OpenGL/GLUT)

## Known issues and TODOs:
* ~~**DONE:** foreign import wrapper support for x86_64. Upstream didn't implement it~~
* **TODO:** foreign import wrapper support for arm/aarch64, Upstream didn't implement it.
* Only tested under linux. Other platform may work but unsure.

```text
------------------------------------------------------------------------------
 The Hugs 98 system is Copyright (c) Mark P Jones, Alastair Reid, the
 Yale Haskell Group, and the OGI School of Science & Engineering at OHSU,
 1994-2005, All rights reserved.  It is distributed as free software under
 the license in the file "LICENSE", which is included in the distribution.
------------------------------------------------------------------------------
```

