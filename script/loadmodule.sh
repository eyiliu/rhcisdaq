#! /usr/bin/env bash

KO_BASE_NAME="fpgadma"

if [ $# -gt 0 ]
then
    KO_BASE_NAME=$1
    echo "Name of kernel module : <${KO_BASE_NAME}>."
else
    echo "Using default name of kernel module : <${KO_BASE_NAME}>."
fi


SCRIPT_BASE_NAME=$(basename $0)

CURRENT_DIR=$(pwd)


SCRIPT_PATH=${BASH_SOURCE}
SCRIPT_DIR=$(dirname ${SCRIPT_PATH})

SCRIPT_REAL_PATH=$(readlink -m ${SCRIPT_PATH})
SCRIPT_REAL_DIR=$(dirname ${SCRIPT_REAL_PATH})

KO_PATH=

if [ ! -n "${KO_PATH-}" ]
then
    KO_CONDIDATE_PATH=${CURRENT_DIR}/${KO_BASE_NAME}.ko
    echo "Search for ${KO_CONDIDATE_PATH}"
    if [ -f ${KO_CONDIDATE_PATH} ]
    then
	echo ".......... Found"
	KO_PATH=${KO_CONDIDATE_PATH}
    fi
fi


if [ ! -n "${KO_PATH-}" ]
then
    KO_CONDIDATE_PATH=${CURRENT_DIR}/${KO_BASE_NAME}.ko
    echo "Search for ${KO_CONDIDATE_PATH}"
    if [ -f ${KO_CONDIDATE_PATH} ]
    then
	echo ".......... Found"
	KO_PATH=${KO_CONDIDATE_PATH}
    fi
fi

if [ ! -n "${KO_PATH-}" ]
then
    KO_CONDIDATE_PATH=${SCRIPT_DIR}/${KO_BASE_NAME}.ko
    echo "Search for ${KO_CONDIDATE_PATH}"
    if [ -f ${KO_CONDIDATE_PATH} ]
    then
	echo ".......... Found"
	KO_PATH=${KO_CONDIDATE_PATH}
    fi
fi

if [ ! -n "${KO_PATH-}" ]
then
    KO_CONDIDATE_PATH=${SCRIPT_REAL_DIR}/${KO_BASE_NAME}.ko
    echo "Search for ${KO_CONDIDATE_PATH}"
    if [ -f ${KO_CONDIDATE_PATH} ]
    then
	echo ".......... Found"
	KO_PATH=${KO_CONDIDATE_PATH}
    fi
fi

if [ ! -n "${KO_PATH-}" ]
then
   KO_CONDIDATE_PATH=$(dirname ${SCRIPT_REAL_DIR})/module/${KO_BASE_NAME}.ko
   echo "Search for ${KO_CONDIDATE_PATH}"
   if [ -f ${KO_CONDIDATE_PATH} ] 
   then
       echo ".......... Found"
       KO_PATH=${KO_CONDIDATE_PATH}
   fi
fi

if [ ! -n "${KO_PATH-}" ]
then
    echo "ERROR: Unable to find ${KO_BASE_NAME}.ko"
fi

echo "Loading kernel module: ${KO_PATH}"
insmod ${KO_PATH}
