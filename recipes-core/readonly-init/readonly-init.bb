SUMMARY = "readonly guest boot setup: 9p mounts + networks"
LICENSE = "APACHE"

SRC_URI = "file://readonly-setup"

inherit update-rc.d

INITSCRIPT_NAME = "readonly-setup"
INITSCRIPT_PARAMS = "start 40 S ."

do_install() {
    install -d ${D}${sysconfdir}/init.d
    install -m 0755 ${WORKDIR}/readonly-setup ${D}${sysconfdir}/init.d/readonly-setup
}
