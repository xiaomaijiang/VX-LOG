#!/bin/bash

NAME=nxlog
TOPDIR=`pwd`/rpmbuild
DISTRO=RHEL`cut -d. -f1 /etc/redhat-release | rev | cut -d' ' -f1`

echo "INFO: build distribution is ${DISTRO}"

#move the spec file, if not already done by jenkins
if [ -f nxlog.spec.${DISTRO}.in ]; then
    mv nxlog.spec.${DISTRO}.in nxlog.spec.in
fi

rm -rf $TOPDIR/BUILD

VERSION=`ls -d nxlog-ce-*.*.*.tar.gz | head -n 1 | sed 's/nxlog-ce-\(.*\).tar.gz/\1/'`
MKDIRLIST="$TOPDIR/BUILD/$NAME-root $TOPDIR/RPMS $TOPDIR/SOURCES $TOPDIR/SPECS $TOPDIR/SRPMS"

for dirn in $MKDIRLIST; do
    mkdir -p $dirn
done

cat $NAME.spec.in | sed s/@VERSION@/$VERSION/ > $NAME.spec

if test x$SPEC_FILE = 'x'; then
    SPEC_FILE="$NAME.spec"
fi

RPM_SPEC="$TOPDIR/SPECS/$SPEC_FILE"

cp nxlog-*.tar.gz $TOPDIR/SOURCES/
cp $SPEC_FILE $RPM_SPEC

cd "$TOPDIR/SPECS/"

rpmbuild -bb --define="_topdir $TOPDIR" --define="_tmppath $TOPDIR/tmp" --buildroot=$TOPDIR/BUILD/$NAME-root $SPEC_FILE

#echo "Cleaning up tempfiles"
#echo "rm -r $TOPDIR"
#rm -r $TOPDIR



