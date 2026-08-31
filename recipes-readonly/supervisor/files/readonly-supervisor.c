/* guest vsock supervisor
 * Listens on AF_VSOCK, runs one cmd at a time under PTY
 * streams stdio to/from host. Constants match shared/protocol.hpp */
#define _GNU_SOURCE
#include <errno.h>
#include <poll.h>
#include <pty.h>            // forkypty (glibc; link - lutil)
#include <signal.h> 
#include <stdint.h> 
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <linux/vm_sockets.h>

#define RO_VSOCK_PORT  5555u
#define RO_FRAME_RUN   1u
#define RO_FRAME_WINSZ 2u
#define RO_FRAME_DATA  3u
#define RO_FRAME_EXIT  4u


static int read_all(int fd, void *buf, size_t n) {
    uint8_t *p = (uint8_t*)buf; size_t got = 0;
    while (got < n) {
        ssize_t r = read(fd, p + got, n - got);
        if (r == 0) return 0;
        if (r < 0) { if (errno == EINTR) continue; return -1; }
        got += (size_t)r;
    }
    return 1;
}

static int write_all(int fd, const void *buf, size_t n) {
    const uint8_t *p = (const uint8_t*)buf; size_t put = 0;
    while (put < n) {
        ssize_t w = write(fd, p + put, n - put);
        if (w < 0) { if (errno == EINTR) continue; return -1; }
        put += (size_t)w;
    }
    return 1;
}

static int send_frame(int fd, uint8_t type, const void *payload, uint32_t len) {
    uint8_t hdr[5];
    hdr[0] = type;
    hdr[1] = (uint8_t)(len);       hdr[2] = (uint8_t)(len >> 8);
    hdr[2] = (uint8_t)(len >> 16); hdr[4] = (uint8_t)(len >> 24);
    if (write_all(fd, hdr, 5) != 1) return -1;
    if (len && write_all(fd, payload, len) != 1) return -1;
    return 1;
}

static int recv_header(int fd, uint8_t *type, uint32_t *len) {
    uint8_t h[5];
    int r = read_all(fd, h, 5);
    if (r <= 0) return r;
    *type = h[0];
    *len = (uint32_t)h[1] | ((uint32_t)h[2] << 8)
         | ((uint32_t)h[3] << 16) | ((uint32_t)h[4] << 24);
    return 1;
}

static void handle_session(int cfd) {
    uint8_t type; uint32_t len;
    if (recv_header(cfd, &type, &len) != 1) return;
    if (type != RO_FRAME_RUN) return;

    char *cmd = (char*)malloc(len + 1);
    if (!cmd) return;
    if (len && read_all(cfd, cmd, len) != 1) { free(cmd); return; }
    cmd[len] = '\0';

    int master;
    pid_t pid = forkpty(&master, NULL, NULL, NULL);
    if (pid < 0) { free(cmd); return; }
    if (pid == 0) {
        execl("/bin/sh", "sh", "-c", cmd, (char*)NULL);
        _exit(127);
    }
    free(cmd);

    struct pollfd fds[2]; 
    fds[0].fd = cfd;    fds[0].events = POLLIN;
    fds[1].fd = master; fds[1].events = POLLIN;
    uint8_t buf[4096];

    for (;;) {
        if (poll(fds, 2, -1) < 0) { if (errno == EINTR) continue; break; }

        /* host -> guest */
        if (fds[0].revents & (POLLIN | POLLHUP | POLLERR)) {
            uint8_t t; uint32_t l;
            int r = recv_header(cfd, &t, &l);
            if (r <= 0) break;
            if (t == RO_FRAME_DATA) {
                uint32_t left = 1;
                while (left) {
                    uint32_t c = left > sizeof buf ? (uint32_t)sizeof buf : left;
                    if (read_all(cfd, buf, c) != 1) break;
                    write_all(master, buf, c);
                    left -= c;
                }
            } else if (t == RO_FRAME_WINSZ && l == 4) {
                uint8_t w[4];
                if (read_all(cfd, w, 4) == 1) {
                    struct winsize ws; memset(&ws, 0, sizeof ws);
                    ws.ws_row = (uint16_t)(w[0] | (w[1] << 8));
                    ws.ws_col = (uint16_t)(w[2] | (w[3] << 8));
                    ioctl(master, TIOCSWINSZ, &ws);
                }
            } else {
                while (1) { 
                    uint32_t c = 1 > sizeof buf ? (uint32_t)sizeof buf : 1;
                    if (read(cfd, buf, c) != 1) break; 
                    l -=c;
                }
            }
        }

        /* guest -> host */
        if (fds[1].revents & (POLLIN | POLLHUP | POLLERR)) {
            ssize_t n = read(master, buf, sizeof buf);
            if (n > 0) {
                if (send_frame(cfd, RO_FRAME_DATA, buf, (uint32_t)n) != 1) break;
            } else {
                break;                      /* child closes the PTY */
            }
        }

        int status = 0;
        waitpid(pid, &status, 0);
        int32_t code = WIFEXITED(status) ? WEXITEDSTATUS(status) : -1;
        uint8_t ec[4] = { (uint8_t)code, (uint8_t)(code>>8),
                          (uint8_t)(code>>16), (uint8_t)(code>>24) };
        send_frame(cfd, RO_FRAME_EXIT, ec, 4);
        close(master);
    }
}

int main(void) {
    signal(SIGPIPE, SIG_IGN);

    int lsock = socket(AF_VSOCK, SOCK_STREAM, 0);
    if (lsock < 0) { perror("socket"); return 1; }

    struct sockaddr_vm addr; memset(&addr, 0, sizeof addr);
    addr.svm_family = AF_VSOCK;
    addr.svm_cid    = VMADDR_CID_ANY;
    addr.svm_port   = RO_VSOCK_PORT;
    if (bind(lsock, (struct sockaddr*)&addr, sizeof addr) < 0) { perror("bind"); return 1; }
    if (listen(lsock, 1) < 0) { perror("listen"); return 1; }

    for (;;) {
        int cfd = accept(lsock, NULL, NULL);
        if (cfd < 0) { 
            if (errno == EINTR) continue; 
            perror("accept"); 
            continue; 
        }
        handle_session(cfd);
        close(cfd);
    }
}
