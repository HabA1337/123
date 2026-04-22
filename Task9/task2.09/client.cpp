#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cctype>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

#define RBUF_SZ   65536
#define CMD_SZ    65536
#define CHUNK_SZ  65536

static int read_full(int fd, void* buf, int n) {
    char* p = (char*)buf;
    int rem = n;
    while (rem > 0) {
        int r = read(fd, p, rem);
        if (r <= 0) return -1;
        p += r;
        rem -= r;
    }
    return n;
}

static int write_full(int fd, const void* buf, int n) {
    const char* p = (const char*)buf;
    int rem = n;
    while (rem > 0) {
        int w = write(fd, p, rem);
        if (w <= 0) return -1;
        p += w;
        rem -= w;
    }
    return n;
}

static int send_command(int fd, const char* cmd, int len) {
    struct iovec iov[2];
    iov[0].iov_base = &len;
    iov[0].iov_len  = sizeof(int);
    iov[1].iov_base = (void*)cmd;
    iov[1].iov_len  = (size_t)len;
    ssize_t total = (ssize_t)sizeof(int) + len;
    ssize_t written = 0;
    while (written < total) {
        ssize_t w = writev(fd, iov, 2);
        if (w <= 0) return -1;
        written += w;
        if (written >= total) break;
        ssize_t rem = w;
        for (int i = 0; i < 2 && rem > 0; i++) {
            if ((size_t)rem >= iov[i].iov_len) {
                rem -= iov[i].iov_len;
                iov[i].iov_len  = 0;
                iov[i].iov_base = nullptr;
            } else {
                iov[i].iov_base = (char*)iov[i].iov_base + rem;
                iov[i].iov_len -= rem;
                rem = 0;
            }
        }
    }
    return 0;
}

static int recv_response(int fd, int* count) {
    int hdr[2];
    if (read_full(fd, hdr, sizeof(hdr)) < 0) return -1;
    *count = hdr[0];
    int text_len = hdr[1];

    char buf[CHUNK_SZ];
    int remaining = text_len;
    while (remaining > 0) {
        int chunk = remaining < CHUNK_SZ ? remaining : CHUNK_SZ;
        if (read_full(fd, buf, chunk) < 0) return -1;
        fwrite(buf, 1, chunk, stdout);
        remaining -= chunk;
    }
    return text_len;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s host port\n", argv[0]);
        return 1;
    }

    static char stdout_buf[1 << 20];
    setvbuf(stdout, stdout_buf, _IOFBF, sizeof(stdout_buf));

    const char* host = argv[1];
    int port = atoi(argv[2]);

    struct hostent* hostinfo = gethostbyname(host);
    if (!hostinfo) {
        fprintf(stderr, "Unknown host: %s\n", host);
        return 1;
    }

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Client: socket");
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr = *(struct in_addr*)hostinfo->h_addr;

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Client: connect");
        return 1;
    }

    int flag = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
               (const void*)&flag, sizeof(flag));

    int res = 0;
    clock_t start = clock();
    bool done = false;

    char rbuf[RBUF_SZ];
    char cmd[CMD_SZ];
    int clen = 0;

    while (!done) {
        int n = read(STDIN_FILENO, rbuf, RBUF_SZ);
        if (n <= 0) break;

        for (int i = 0; i < n && !done; i++) {
            if (clen < CMD_SZ - 1)
                cmd[clen++] = rbuf[i];

            if (rbuf[i] == ';') {
                cmd[clen] = '\0';

                bool is_empty = true;
                for (int j = 0; j < clen; j++) {
                    unsigned char ch = (unsigned char)cmd[j];
                    if (!isspace(ch) && ch != ';') { is_empty = false; break; }
                }

                if (!is_empty) {
                    if (send_command(sock, cmd, clen) < 0) {
                        done = true;
                        break;
                    }

                    int count;
                    if (recv_response(sock, &count) < 0) {
                        done = true;
                        break;
                    }

                    if (count < 0) {
                        done = true;
                    } else {
                        res += count;
                    }
                }
                clen = 0;
            }
        }
    }

    close(sock);

    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    printf("%s : Result = %d Elapsed = %.2f\n", argv[0], res, elapsed);

    return 0;
}
