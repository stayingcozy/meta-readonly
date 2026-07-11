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
