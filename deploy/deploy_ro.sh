#!/bin/bash


DEVDIR="/dev/x86_adapt/"


DEVMODE=0644  # root can read/write, the rest should only read
DIRMODE=0755  # directories readable and executable for everyone, writable only for root 

SCRIPTPATH=$( cd $(dirname $0) ; pwd -P ) 

KMODNAME=x86_adapt_driver
KMODPATH=${SCRIPTPATH}/../build/${KMODNAME}.ko


# first, see if the module is still loaded
if  lsmod | grep ${KMODNAME} &> /dev/null
then
	echo "-> Unloading module ${KMODNAME}"
	rmmod ${KMODNAME} || exit 1
fi

# second, load the module
echo "-> Loading module ${KMODNAME}"
insmod ${KMODPATH} || exit 1

# third, set permissions 
IDENT="  "
echo "Setting permissions:"
## 1) on directories
for d in $(find ${DEVDIR} -type d)
do
	echo "${IDENT}-> chmod ${d} to ${DIRMODE}"
	chmod ${DIRMODE} ${d} || exit 1
done

for f in $(find ${DEVDIR} -type c)
do
	echo "${IDENT}-> chmod ${f} to ${DEVMODE}"
	chmod ${DEVMODE} ${f} || exit 1
done

echo
echo "Done."
