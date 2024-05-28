#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
ASSIGNMENT_DIRECTORY=/home/velascor/assignment-1-velascorboulder/finder-app
SYSROOT_DIR=/usr/local/arm-cross-compiler/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # _TODO: Add your kernel build steps here 
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

mkdir -p rootfs
cd rootfs

# _TODO: Create necessary base directories
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # _TODO:  Configure busybox
    make distclean
    make defconfig
else
    cd busybox
fi

# _TODO: Make and install busybox
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

cd ${OUTDIR}/rootfs
echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

#cd ${FINDER_APP_DIR}
# _TODO: Add library dependencies to rootfs
cp ${FINDER_APP_DIR}/ccdependencies/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib
cp ${FINDER_APP_DIR}/ccdependencies/libc.so.6 ${OUTDIR}/rootfs/lib64
cp ${FINDER_APP_DIR}/ccdependencies/libm.so.6 ${OUTDIR}/rootfs/lib64
cp ${FINDER_APP_DIR}/ccdependencies/libresolv.so.2 ${OUTDIR}/rootfs/lib64

#${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter" | awk -F': ' '{print $2}' | awk -v base="$SYSROOT_DIR" -F']' '{print base $1}' | xargs -I {} cp {} lib

#${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library" | awk -F'[' '{print $2}' | awk -v base="$SYSROOT_DIR"/lib64/ -F']' '{print base $1}' | xargs -I {} cp {} lib64


# _TODO: Make device nodes
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 666 dev/console c 5 1

# _TODO: Clean and build the writer utility
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE}

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp autorun-qemu.sh ${OUTDIR}/rootfs/home
cp finder-test.sh ${OUTDIR}/rootfs/home
cp writer ${OUTDIR}/rootfs/home
cp finder.sh ${OUTDIR}/rootfs/home
cp -r conf/ ${OUTDIR}/rootfs/home

OLD_LINE="assignment='cat ../conf/assignment.txt'"
NEW_LINE="assignment=$(cat conf/assignment.txt)"
sed -i "35c\\${NEW_LINE}" ${OUTDIR}/rootfs/home/finder-test.sh

# TODO: Chown the root directory
cd ${OUTDIR}/rootfs
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio

# TODO: Create initramfs.cpio.gza
cd ${OUTDIR}
gzip -f initramfs.cpio
