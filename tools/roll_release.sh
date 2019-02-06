#!/usr/bin/env bash
#
# Copyright (c) 2011-2012 Bryce Adelstein-Lelbach
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file BOOST_LICENSE_1_0.rst or copy at http://www.boost.org/LICENSE_1_0.txt)

VERSION_MAJOR=`grep '#define HPX_VERSION_MAJOR' hpx/config/version.hpp | awk {' print $3 '}`
VERSION_MINOR=`grep '#define HPX_VERSION_MINOR' hpx/config/version.hpp | awk {' print $3 '}`
VERSION_SUBMINOR=`grep '#define HPX_VERSION_SUBMINOR' hpx/config/version.hpp | awk {' print $3 '}`
VERSION_TAG=`grep '#define HPX_VERSION_TAG' hpx/config/version.hpp | awk {' print $3 '} | tr --delete '"'`

if [ ! -z "$VERSION_TAG" ]; then
    echo "Warning: VERSION_TAG is not empty (\"$VERSION_TAG\")."
    echo "If you intended to make a final release, remove the tag in hpx/config/version.hpp."
fi

DOT_VERSION=$VERSION_MAJOR.$VERSION_MINOR.$VERSION_SUBMINOR$VERSION_TAG
UNDERSCORE_VERSION=${VERSION_MAJOR}_${VERSION_MINOR}_$VERSION_SUBMINOR$VERSION_TAG

DOCS_WEBSITE="https://stellar-group.github.io"
SOURCE_WEBSITE="http://stellar.cct.lsu.edu"

ZIP=hpx_$DOT_VERSION.zip
TARGZ=hpx_$DOT_VERSION.tar.gz
TARBZ2=hpx_$DOT_VERSION.tar.bz2
SEVENZ=hpx_$DOT_VERSION.7z

rm -rf packages
mkdir -p packages/zip/hpx_$DOT_VERSION
mkdir -p packages/tar.gz/hpx_$DOT_VERSION
mkdir -p packages/tar.bz2/hpx_$DOT_VERSION
mkdir -p packages/7z/hpx_$DOT_VERSION

echo -n "Packaging $ZIP... "
zip -q -x .git\* -x packages -x packages/\* -r packages/$ZIP .
(cd packages/zip/hpx_$DOT_VERSION && unzip -qq ../../$ZIP)
rm -f packages/$ZIP
(cd packages/zip && zip -q -r ../$ZIP hpx_$DOT_VERSION)
rm -rf packages/zip/hpx_$DOT_VERSION
(cd packages/zip && unzip -qq ../$ZIP)
echo "DONE"

echo -n "Packaging $TARGZ... "
tar --exclude=.git\* --exclude=packages --exclude=packages/\* -czf packages/$TARGZ .
(cd packages/tar.gz/hpx_$DOT_VERSION && tar -xf ../../$TARGZ)
rm -f packages/$TARGZ
(cd packages/tar.gz && tar -czf ../$TARGZ hpx_$DOT_VERSION)
rm -rf packages/tar.gz/hpx_$DOT_VERSION
(cd packages/tar.gz && tar -xf ../$TARGZ)
echo "DONE"

echo -n "Packaging $TARBZ2... "
tar --exclude=.git\* --exclude=packages --exclude=packages/\* -cjf packages/$TARBZ2 .
(cd packages/tar.bz2/hpx_$DOT_VERSION && tar -xf ../../$TARBZ2)
rm -f packages/$TARBZ2
(cd packages/tar.bz2 && tar -cjf ../$TARBZ2 hpx_$DOT_VERSION)
rm -rf packages/tar.bz2/hpx_$DOT_VERSION
(cd packages/tar.bz2 && tar -xf ../$TARBZ2)
echo "DONE"

if type -t "7za" > /dev/null;
then
	SEVENZIP=7za
else
	SEVENZIP=7zr
fi
echo -n "Packaging $SEVENZ... "
$SEVENZIP a -xr\!.git -xr\!packages packages/$SEVENZ . > /dev/null
(cd packages/7z/hpx_$DOT_VERSION && $SEVENZIP x ../../$SEVENZ > /dev/null)
rm -f packages/$SEVENZ
(cd packages/7z && $SEVENZIP a ../$SEVENZ hpx_$DOT_VERSION > /dev/null)
rm -rf packages/7z/hpx_$DOT_VERSION
(cd packages/7z && $SEVENZIP x ../$SEVENZ > /dev/null)
echo "DONE"

if [ ! -z "$VERSION_TAG" ]; then
    echo "Not printing HTML for non-final release."
    exit
fi

ZIP_MD5=`md5sum packages/$ZIP | awk {'print $1'}`
TARGZ_MD5=`md5sum packages/$TARGZ | awk {'print $1'}`
TARBZ2_MD5=`md5sum packages/$TARBZ2 | awk {'print $1'}`
SEVENZ_MD5=`md5sum packages/$SEVENZ | awk {'print $1'}`

ZIP_SIZE=`ls -s -h packages/$ZIP | awk {'print $1'}`
TARGZ_SIZE=`ls -s -h packages/$TARGZ | awk {'print $1'}`
TARBZ2_SIZE=`ls -s -h packages/$TARBZ2 | awk {'print $1'}`
SEVENZ_SIZE=`ls -s -h packages/$SEVENZ | awk {'print $1'}`

echo "<ul>"
echo "  <li>HPX V$DOT_VERSION: <a title=\"HPX V$DOT_VERSION Release Notes\" href=\"$DOCS_WEBSITE/hpx/docs/sphinx/tags/$DOT_VERSION/html/releases/whats_new_$UNDERSCORE_VERSION.html\">release notes</a>"
echo "  <table>"
echo "    <tr><th>File</th><th>MD5 Hash</th></tr>"
echo "    <tr><td><a title=\"HPX V$DOT_VERSION (zip)\" href=\"$SOURCE_WEBSITE/files/$ZIP\">zip ($ZIP_SIZE)</a></td><td><code>$ZIP_MD5</code></td></tr>"
echo "    <tr><td><a title=\"HPX V$DOT_VERSION (gz)\" href=\"$SOURCE_WEBSITE/files/$TARGZ\">gz ($TARGZ_SIZE)</a></td><td><code>$TARGZ_MD5</code></td></tr>"
echo "    <tr><td><a title=\"HPX V$DOT_VERSION (bz2)\" href=\"$SOURCE_WEBSITE/files/$TARBZ2\">bz2 ($TARBZ2_SIZE)</a></td><td><code>$TARBZ2_MD5</code></td></tr>"
echo "    <tr><td><a title=\"HPX V$DOT_VERSION (7z)\" href=\"$SOURCE_WEBSITE/files/$SEVENZ\">7z ($SEVENZ_SIZE)</a></td><td><code>$SEVENZ_MD5</code></td></tr>"
echo "  </table>"
echo "  </li>"
echo "</ul>"

