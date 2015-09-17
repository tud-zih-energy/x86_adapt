#!/bin/bash

#remove old directory
if [ -d debpack ]; then
    rm -r debpack
fi

#copy template
cp -r debpack_template debpack

#set revision
if [ -z "$1" ]; then
    REVISION=`svn info | grep Revision | cut -d " " -f 2`
else
    REVISION=$1
fi
echo $REVISION
sed -i "s/###VERSION###/$REVISION/g" debpack/DEBIAN/*
sed -i "s/###VERSION###/$REVISION/g" debpack/usr/src/x86_adapt_driver-template/dkms.conf

#copy files
cp x86_adapt_driver_template.c debpack/usr/src/x86_adapt_driver-template/
cp *.py debpack/usr/src/x86_adapt_driver-template/
cp -r knobs debpack/usr/src/x86_adapt_driver-template/
cp -r processors debpack/usr/src/x86_adapt_driver-template/

#rename dkms directory
mv debpack/usr/src/x86_adapt_driver-template/ debpack/usr/src/x86_adapt_driver-$REVISION

#build deb
dpkg -b ./debpack x86_adapt_driver.deb
