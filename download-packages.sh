cpphs_ver=1.11
hsc2hs_ver=0.67.20061107
happy_ver=1.18.4

#create dir, just in case.
mkdir -p packages

#download packages from hackage.haskell.org
for pkg in `cat packages.list`; 
do
  echo "Download $pkg:"
  wget http://hackage.haskell.org/package/$pkg/$pkg.tar.gz -O packages/$pkg.tar.gz
done

#extract packages.
for pkg in `cat packages.list`;
do
  pkgname=`echo $pkg|awk 'BEGIN{FS=OFS="-"}{NF--; print}'`
  echo "Extract $pkg.tar.gz to packages/$pkgname"
  mkdir -p packages/$pkgname
  tar xf packages/$pkg.tar.gz -C packages/$pkgname --strip-component=1
done 

rm -rf packages/*.tar.gz

#fix perms in case.
find packages -name configure|xargs chmod +x

#apply patches for packages.
cat packages/patches/cabal-001-find-hsc2hs-and-cpphs-with-hugs-suffix.patch|patch -p1 -d packages/Cabal
cat packages/patches/cabal-002-find-happy-with-hugs-suffix.patch|patch -p1 -d packages/Cabal
cat packages/patches/directory-001-no-depend-on-unix.patch|patch -p1 -d packages/directory
cat packages/patches/HGL-001-fix-with-new-base.patch|patch -p1 -d packages/HGL
cat packages/patches/network-001-fix-ucred_struct.patch|patch -p1 -d packages/network
cat packages/patches/network-002-fix-inline-link-issue.patch|patch -p1 -d packages/network
cat packages/patches/unix-001-add-c_rename.patch|patch -p1 -d packages/unix
cat packages/patches/unix-002-fix-inline-link-issue.patch|patch -p1 -d packages/unix



#download and extract cpphs
wget http://hackage.haskell.org/package/cpphs-$cpphs_ver/cpphs-$cpphs_ver.tar.gz
mkdir -p cpphs
tar xf cpphs-$cpphs_ver.tar.gz -C cpphs --strip-component=1
rm -rf cpphs-$cpphs_ver.tar.gz

#download and extract hsc2hs
wget http://hackage.haskell.org/package/hsc2hs-$hsc2hs_ver/hsc2hs-$hsc2hs_ver.tar.gz
mkdir -p hsc2hs
tar xf hsc2hs-$hsc2hs_ver.tar.gz -C hsc2hs --strip-component=1
rm -rf hsc2hs-$hsc2hs_ver.tar.gz 

#download and extract happy
#note, DO NOT delete happy dir, it contains patches and preprocess files.
wget http://hackage.haskell.org/package/happy-$happy_ver/happy-$happy_ver.tar.gz
tar xf happy-$happy_ver.tar.gz -C happy --strip-component=1
rm -rf happy-$happy_ver.tar.gz 

#apply patch for happy
cat happy/patches/happy-001-use-cpphs-hugs-to-generate-templates.patch|patch -p1 -d happy

#get base 4.0
cd packages; darcs get -t "6.10 branch has been forked"  http://darcs.haskell.org/packages/base;rm -rf base/_darcs
