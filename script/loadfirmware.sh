#! /usr/bin/env sh

DTBO_BASE_NAME="fpgafw"
if [ $# -gt 0 ]
then
    DTBO_BASE_NAME=$1
    echo "Using default name of device-tree-overlay: <${DTBO_BASE_NAME}>."
fi

echo "Search device-tree-overlay file: <${DTBO_BASE_NAME}>."
DTBO_FILE_PATH=/usr/lib/firmware/${DTBO_BASE_NAME}.dtbo
if [ ! -f ${DTBO_FILE_PATH} ]
then
    echo "ERROR: Unable to find device-tree-overlay file at path: <${DTBO_FILE_PATH}>"
    exit -1
fi

CONFIGFS_DTBO_FULL_PATH=/configfs/device-tree/overlays/full
if [ -d ${CONFIGFS_DTBO_FULL_PATH} ]
then
    echo "Overlay config path exists at configfs. Removing"
    rmdir ${CONFIGFS_DTBO_FULL_PATH}
fi

echo "Configure fpga manager"
SYSFS_FPGA_PATH=/sys/class/fpga_manager/fpga0
if [ -d ${SYSFS_FPGA_PATH} ]
then 
    echo 0 > ${SYSFS_FPGA_PATH}/flags
else
    echo "ERROR: fpga manager is not avalible. Please enable it in the kernel."  
    exit -1
fi

echo "Configure configfs"
FINDMNT_RE=$(findmnt -T /configfs -t configfs)
if [ $? != 0 ]
then
    echo "Configfs does not exist. Mounting configfs"
    mount -t configfs configfs /configfs
    if [ ! -d /configfs ]
       echo "ERROR: Unable to mount configfs. Please enable it in the kernel."
       exit -1
    then
else
    echo "Configfs exists."
fi

mkdir -p ${CONFIGFS_DTBO_FULL_PATH}

echo "Loading device-tree-overlay: <${DTBO_BASE_NAME}>"
echo -n "${DTBO_BASE_NAME}.dtbo" > ${CONFIGFS_DTBO_FULL_PATH}/path
