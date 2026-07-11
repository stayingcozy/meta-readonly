SUMMARY = "readonly vsock supervisor (placeholder)"
LICENSE = "CLOSED"

SRC_URI = "file://readonly-supervisor.sh"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/readonly-supervisor.sh ${D}${bindir}/readonly-supervisor
}
