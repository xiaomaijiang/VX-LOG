#!/bin/sh

DEBDIR="../../debian"
VERSION=`cd ../.. && ./version.sh|sed s/\:/_/`
DISTRO='default';
if test x`which lsb_release` != "x"; then
     DISTROUC="`lsb_release -i -s`-`lsb_release -c -s`";
 DISTRO="`echo $DISTROUC|tr '[:upper:]' '[:lower:]'`";
else
    echo "please install the lsb-release package";
    exit 1;
fi
ARCH=`dpkg --print-architecture`

(
    
#    if echo $VERSION | grep \: >/dev/null; then
#	echo "refusing to build package, changes must be committed to svn first"
#	exit 1
#    fi

     if test -f control.$DISTRO; then 
	cp -f control.$DISTRO control
     else
	echo "control.$DISTRO not found, using control.default to build package"
	cp -f control.default control
     fi

    RELEASE_DATE=`LC_ALL=en_US date "+%a, %d %b %Y %T %z"`
    rm -f changelog
    echo "nxlog-ce ($VERSION) unstable; urgency=low" >changelog
    echo "" >>changelog
    echo "  * SVN snapshot release." >>changelog
    echo "" >>changelog
    echo " -- Botond Botyanszki <boti@nxlog.org>  $RELEASE_DATE" >>changelog
    echo "" >>changelog
    cat changelog.skel >>changelog

    cd ../..
    ln -s -f packaging/debian debian

#    export DEB_BUILD_OPTIONS=nostrip,noopt
    dpkg-buildpackage -b -rfakeroot || exit 2;
    if test "x$RUN_TESTS" = "x1";
	then make check || exit 2;
    fi

)
RC=$?
rm -f $DEBDIR
if test -f changelog; then
    rm -f changelog
fi

rm -f control
exit $RC
