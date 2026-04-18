#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cctype>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define RBUF_SZ  4096
#define CMD_SZ   65536
#define CHUNK_SZ 4096

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
    if (write_full(fd, &len, sizeof(int)) < 0) return -1;
    if (write_full(fd, cmd, len) < 0) return -1;
    return 0;
}

static int recv_response(int fd, int* count) {
    if (read_full(fd, count, sizeof(int)) < 0) return -1;
    int text_len;
    if (read_full(fd, &text_len, sizeof(int)) < 0) return -1;

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
