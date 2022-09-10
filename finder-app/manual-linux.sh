#!/bin/bash

# PATH=$PATH:/home/lda/Downloads/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/bin

# set -e
set -u

OUTDIR=~/Downloads/aeld
ASSIGNMENTDIR=/home/lda/OneDrive/linux_system/assignment-1-LDAnderson/finder-app
# ASSIGNMENTDIR=/home/ace/assignment-1-LDAnderson/finder-app


 # git clone git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git --depth 1 --single-branch --branch v5.1.10
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi
ROOTDIR=${OUTDIR}/rootfs

mkdir -p ${OUTDIR}

if [ ! -d "${OUTDIR}" ]
   then
   echo "Error creating output directory"
   exit 1
fi


cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --single-branch --branch ${KERNEL_VERSION}
fi

if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}


    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    # make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig virt

    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    sed -i 's/^YYLTYPE yylloc;/extern YYLTYPE yylloc;/' ${OUTDIR}/linux-stable/scripts/dtc/dtc-lexer.lex.c
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs

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

mkdir -p ${ROOTDIR}
if [ ! -d "${ROOTDIR}" ]
   then
   echo "Error creating root directory"
   exit 1
fi

# make filesystem
cd ${ROOTDIR}
mkdir bin dev etc home lib proc sbin sys tmp usr var
mkdir usr/bin usr/lib usr/sbin
mkdir -p var/log

# set up busy box

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    make distclean
    make defconfig
    # sudo make ARCH=arm CROSS_COMPILE=arm-unknown-linux-gnu-eabi- install
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} CONFIG_PREFIX=${ROOTDIR} install
    # cd _install
    # sudo cp -a * ${ROOTDIR}

else
    cd busybox
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} CONFIG_PREFIX=${ROOTDIR} install
fi

echo "Library dependencies"
cd ${ROOTDIR}
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# sudo cp /home/lda/Downloads/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/bin/../aarch64-none-linux-gnu/libc/lib/ld-linux-aarch64.so.1 lib
# sudo cp /home/lda/Downloads/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/bin/../aarch64-none-linux-gnu/libc/lib64/libm.so.6 lib
# sudo cp /home/lda/Downloads/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/bin/../aarch64-none-linux-gnu/libc/lib64/libresolv.so.2 lib
# sudo cp /home/lda/Downloads/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/bin/../aarch64-none-linux-gnu/libc/lib64/libc.so.6 lib

SYSROOT=`aarch64-none-linux-gnu-gcc -print-sysroot`

cd ${ROOTDIR}
cp -a "${SYSROOT}"/lib/* lib/
cp -a "${SYSROOT}"/lib64 .

#  Make device nodes
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1

# install modules
# cd ${OUTDIR}/linux-stable
# sudo make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} INSTALL_MOD_PATH=${ROOTDIR} modules_install

# Clean and build the writer utility
cd ${ASSIGNMENTDIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE}
sudo cp writer ${ROOTDIR}/home

#  Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cd ${ROOTDIR}
cd home
mkdir conf
cd ${ASSIGNMENTDIR}
sudo cp finder.sh ${ROOTDIR}/home
sudo cp conf/username.txt ${ROOTDIR}/home/conf
sudo cp finder-test.sh ${ROOTDIR}/home
sudo cp autorun-qemu.sh ${ROOTDIR}/home

#  Chown the root directory
cd ${ROOTDIR}
sudo chown -R root:root .

# Create initramfs.cpio.gz
cd ${ROOTDIR}
find . | sudo cpio -o -H newc  > ../initramfs.cpio
cd ..
gzip -f initramfs.cpio

# QEMU_AUDIO_DRV=none qemu-system-arm -m 256M -nographic -M versatilepb -kernel zImage -append "console=ttyAMA0 rdinit=/bin/sh" -dtb versatile-pb.dtb -initrd initramfs.cpio.gz

# qemu-system-arm -m 256M -nographic -M versatilepb -kernel Image
