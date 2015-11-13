#!/bin/sh

VERSION="0.3"
DEF_VERSION="0.1"
DRIVER="x86_adapt_driver"
DEF_DRIVER="x86_adapt_defs"

DRIVER_TARGET=${DRIVER}-${VERSION}
DEF_DRIVER_TARGET=${DEF_DRIVER}-${DEF_VERSION}

#clean up old
rm -rf ${DRIVER}*
rm -rf ${DEF_DRIVER}*

#make folder for definitions driver
mkdir ${DEF_DRIVER_TARGET}
mkdir ${DEF_DRIVER_TARGET}/src
mkdir ${DEF_DRIVER_TARGET}/definition_driver

#create definitions driver source
../definition_driver/prepare.py ${DEF_DRIVER_TARGET}/src ../definition_driver/

#copy header
cp ../definition_driver/x86_adapt_defs.h  ${DEF_DRIVER_TARGET}/definition_driver/

# copy makefile and dkms.conf
cp files/definition_driver/Makefile ${DEF_DRIVER_TARGET}/src
cp files/definition_driver/dkms.conf ${DEF_DRIVER_TARGET}

#make folder for driver
mkdir ${DRIVER_TARGET}

cp -r ../driver ${DRIVER_TARGET}/src
mkdir ${DRIVER_TARGET}/definition_driver
../definition_driver/prepare.py ${DRIVER_TARGET}/definition_driver ../definition_driver/
cp ../definition_driver/x86_adapt_defs.h ${DRIVER_TARGET}/definition_driver/
cp files/definition_driver/Makefile ${DRIVER_TARGET}/definition_driver/
cp files/driver/Makefile ${DRIVER_TARGET}/src
cp files/driver/dkms.conf ${DRIVER_TARGET}/

#drop permissions
chmod -R og-rwx ${DRIVER_TARGET}
chmod -R og-rwx ${DEF_DRIVER_TARGET}

# change owner
sudo chown root:root ${DRIVER_TARGET}
sudo chown root:root ${DEF_DRIVER_TARGET}
