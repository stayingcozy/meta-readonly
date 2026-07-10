SUMMARY = "readonly base userspace tools"
LICENSE = "APACHE"

inherit packagegroup

RDEPENDS:${PN} = "busybox \
                  bash \
                  curl \
                  ca-certificates \
                  git"
