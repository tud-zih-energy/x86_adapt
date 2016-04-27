#!/bin/bash
# this script generates a debian/ubuntu package with dkms support

#remove old directory
if [ -d debpack ]; then
    rm -r debpack
fi

#copy template
cp -r debpack_template debpack

#set revision
if [ -z "$1" ]; then
    REVISION=$(git log -1 --date=short --pretty=format:%cd)
else
    REVISION=$1
fi
echo $REVISION
sed -i "s/###VERSION###/$REVISION/g" debpack/DEBIAN/*
sed -i "s/###VERSION###/$REVISION/g" debpack/usr/src/x86_adapt_defs-template/dkms.conf
sed -i "s/###VERSION###/$REVISION/g" debpack/usr/src/x86_adapt_driver-template/*

#create uncore knobs
cd definition_driver/knobs
./write_uncore_pmc_definitions.py
cd -

## x86_adapt_defs
#create definitions driver source
./definition_driver/prepare.py debpack/usr/src/x86_adapt_defs-template/src definition_driver/
#copy header
cp --parents definition_driver/x86_adapt_defs.h debpack/usr/src/x86_adapt_defs-template/

## x86_adapt_driver
#copy files
cp -r driver/. debpack/usr/src/x86_adapt_driver-template/src
# TODO: maybe change make file
./definition_driver/prepare.py debpack/usr/src/x86_adapt_driver-template/definition_driver definition_driver/
cp --parents definition_driver/x86_adapt_defs.h debpack/usr/src/x86_adapt_driver-template/

#rename dkms directory
mv debpack/usr/src/x86_adapt_defs-template/ debpack/usr/src/x86_adapt_defs-$REVISION
mv debpack/usr/src/x86_adapt_driver-template/ debpack/usr/src/x86_adapt_driver-$REVISION

#build deb
dpkg -b ./debpack x86_adapt_driver.deb
