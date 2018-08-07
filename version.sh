#!/bin/sh

VERSION_PATCH='0';
if test -d .git; then
    GITVERSIONCMD=`which git 2>/dev/null`
    if test "X$GITVERSIONCMD" = "X"; then
        echo "git not found";
        exit 1;
    fi
    VERSION_PATCH=`git log --pretty=oneline 2>/dev/null | wc -l`
    echo ${VERSION_PATCH} >svn_version.txt 
else
    SVNVERSIONCMD=`which svnversion 2>/dev/null`
    if test "X$SVNVERSIONCMD" = "X" || test "`$SVNVERSIONCMD -n '.'`" = "exported" || test "`$SVNVERSIONCMD -n '.'`" = "Unversioned directory"; then
        VERSION_PATCH=`cat svn_version.txt`
    else
        VERSION_PATCH=`svnversion -n |sed 's/\:.*$//'`
        echo ${SVN_VERSION} >svn_version.txt 
    fi
fi

VERSION=`cat VERSION`
echo -n "${VERSION}.${VERSION_PATCH}" |sed s/M//
