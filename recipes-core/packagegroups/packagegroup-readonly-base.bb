SUMMARY = "readonly base userspace tools"
LICENSE = "MIT"

inherit packagegroup

RDEPENDS:${PN} = "busybox \
                  bash \
                  curl \
                  ca-certificates \
                  git"
