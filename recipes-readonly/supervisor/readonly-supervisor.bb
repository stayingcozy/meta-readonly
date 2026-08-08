SUMMARY = "readonly vsock supervisor"
LICENSE = "CLOSED"

SRC_URI = "file://readonly-supervisor.c"
S = "${WORKDIR}"

do_compile() {
    ${CC} ${CFLAGS} ${LDFLAGS} -o readonly-supervisor ${S}/readonly-supervisor.c -lutil
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 readonly-supervisor ${D}${bindir}/readonly-supervisor
}
