cpphs_ver=1.11
hsc2hs_ver=0.67.20061107

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

#get base 4.0
cd packages; darcs get -t "6.10 branch has been forked"  http://darcs.haskell.org/packages/base
