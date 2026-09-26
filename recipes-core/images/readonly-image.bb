SUMMARY = "readonly guest base image"
LICENSE = "MIT"

inherit core-image

# packagegroup-core-boot = kernel, init, getty, base userspace (bootable minimum)
IMAGE_INSTALL = "packagegroup-core-boot \
                 packagegroup-readonly-base \
                 readonly-init \
                 readonly-supervisor \
                 ${CORE_IMAGE_EXTRA_INSTALL}"

# ext4 rootfs, then qemu-img convets it to qcow2
IMAGE_FSTYPES = "ext4 ext4.qcow2"

IMAGE_ROOTFS_EXTRA_SPACE = "1048576"

ROOTFS_POSTPROCESS_COMMAND += "create_lib64_compat_symlink; "

create_lib64_compat_symlink () {
    if [ ! -e ${IMAGE_ROOTFS}/lib64 ]; then
        ln -sfn lib ${IMAGE_ROOTFS}/lib64
    fi
}
