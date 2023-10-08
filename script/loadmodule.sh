#! /usr/bin/env bash

BASH_SOURCE_REAL_PATH=$(readlink -m ${BASH_SOURCE})
BASH_SOURCE_DIR=$(dirname ${BASH_SOURCE_REAL_PATH})
BASH_SOURCE_BASE_NAME=$(basename $0)

KO_PATH=${BASH_SOURCE_DIR}/../module/fpgadma.ko
KO_REAL_PATH=$(readlink -m ${KO_PATH})

echo "loading kernel module: ${KO_REAL_PATH}"
insmod ${KO_REAL_PATH}

