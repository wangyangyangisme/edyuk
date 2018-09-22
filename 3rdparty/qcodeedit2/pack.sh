#!/bin/sh

version=$1
package=qcodeedit-$version

mkdir pack-$version
cd pack-$version

# tag
svn copy https://edyuk.svn.sf.net/svnroot/edyuk/trunk/3rdparty/qcodeedit2/ https://edyuk.svn.sf.net/svnroot/edyuk/tags/qce-$version -m "Tagged QCE $version"

# Nix
svn export https://edyuk.svn.sf.net/svnroot/edyuk/trunk/3rdparty/qcodeedit2/ $package --native-eol LF &> /dev/null

tar -cf $package.tar $package/

bzip2 -zk9 $package.tar
gzip -9 $package.tar

rm -rf $package

# Dos
svn export https://edyuk.svn.sf.net/svnroot/edyuk/trunk/3rdparty/qcodeedit2/ $package --native-eol CRLF &> /dev/null

7z a -t7z -m0=lzma -mx=9 $package.7z $package &> /dev/null
7z a -tzip -mx=9 $package.zip $package &> /dev/null

rm -rf $package

cd ..

