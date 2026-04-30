#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <cstring>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cctype>
#include <unistd.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "record.h"
#include "command.h"
#include "record_list.h"
#include "name_index.h"
#include "phone_index.h"
#include "group_index.h"

#define CMD_BUF_SZ 65536

struct strbuf {
    char*  buf;
    size_t len;
    size_t cap;
};

static void sb_init(strbuf& s) {
    s.buf = nullptr;
    s.len = 0;
    s.cap = 0;
}

static void sb_free(strbuf& s) {
    if (s.buf) free(s.buf);
    s.buf = nullptr;
    s.len = 0;
    s.cap = 0;
}

static int sb_reserve(strbuf& s, size_t add) {
    size_t need = s.len + add + 1;
    if (need <= s.cap) return 0;
    size_t nc = s.cap ? s.cap : 4096;
    while (nc < need) nc *= 2;
    char* nb = (char*)realloc(s.buf, nc);
    if (!nb) return -1;
    s.buf = nb;
    s.cap = nc;
    return 0;
}

static void sb_write(strbuf& s, const char* data, size_t n) {
    if (sb_reserve(s, n) < 0) return;
    memcpy(s.buf + s.len, data, n);
    s.len += n;
    s.buf[s.len] = '\0';
}

static void sb_printf(strbuf& s, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    size_t avail = (s.cap > s.len) ? (s.cap - s.len) : 0;
    int n = vsnprintf(s.buf ? s.buf + s.len : nullptr, avail, fmt, ap);
    va_end(ap);
    if (n < 0) return;
    if ((size_t)n + 1 > avail) {
        if (sb_reserve(s, (size_t)n) < 0) return;
        avail = s.cap - s.len;
        va_start(ap, fmt);
        n = vsnprintf(s.buf + s.len, avail, fmt, ap);
        va_end(ap);
        if (n < 0) return;
    }
    s.len += (size_t)n;
}

static void print_rec_to_sb(strbuf& s, const record& r, const ordering* order) {
    static const int max_items = 3;
    static const ordering default_ordering[max_items] = {
        ordering::name, ordering::phone, ordering::group
    };
    const ordering* p = (order ? order : default_ordering);

    for (int i = 0; i < max_items; i++) {
        switch (p[i]) {
            case ordering::name:
                sb_printf(s, "%s", r.get_name() ? r.get_name() : "");
                break;
            case ordering::phone:
                sb_printf(s, "%d", r.get_phone());
                break;
            case ordering::group:
                sb_printf(s, "%d", r.get_group());
                break;
            case ordering::none:
                continue;
        }
        if (i < max_items - 1 && p[i+1] != ordering::none)
            sb_write(s, " ", 1);
    }
    sb_write(s, "\n", 1);
}

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

static int recv_msg(int fd, char* buf, int max_len) {
    int len;
    if (read_full(fd, &len, sizeof(int)) < 0) return -1;
    if (len <= 0 || len >= max_len) return -1;
    if (read_full(fd, buf, len) < 0) return -1;
    buf[len] = '\0';
    return len;
}

static int send_response(int fd, int count, const char* text, int text_len) {
    int hdr[2] = { count, text_len };
    struct iovec iov[2];
    iov[0].iov_base = hdr;
    iov[0].iov_len  = sizeof(hdr);
    iov[1].iov_base = const_cast<char*>(text);
    iov[1].iov_len  = (size_t)text_len;
    int cnt = (text_len > 0) ? 2 : 1;
    ssize_t total = (ssize_t)sizeof(hdr) + (text_len > 0 ? text_len : 0);
    ssize_t written = 0;
    while (written < total) {
        ssize_t w = writev(fd, iov, cnt);
        if (w <= 0) return -1;
        written += w;
        if (written >= total) break;
        ssize_t rem = w;
        for (int i = 0; i < cnt && rem > 0; i++) {
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

static void merge_by_order(list_node** arr, int n, list_node** buf,
                           const ordering* ob) {
    if (n <= 1) return;
    int mid = n / 2;
    merge_by_order(arr, mid, buf, ob);
    merge_by_order(arr + mid, n - mid, buf, ob);
    int i = 0, j = mid, w = 0;
    while (i < mid && j < n) {
        if (command::compare_records(arr[i]->rec, arr[j]->rec, ob) <= 0)
            buf[w++] = arr[i++];
        else
            buf[w++] = arr[j++];
    }
    while (i < mid) buf[w++] = arr[i++];
    while (j < n)   buf[w++] = arr[j++];
    for (int x = 0; x < n; x++) arr[x] = buf[x];
}

static void sort_found(dyn_array<list_node*>& recs, const command& cmd) {
    const ordering* ob = cmd.get_order_by();
    if (ob[0] == ordering::none || recs.size() <= 1) return;
    int n = recs.size();
    list_node** buf = new list_node*[n];
    merge_by_order(recs.data(), n, buf, ob);
    delete[] buf;
}

struct config_params {
    int k_hash_name, k_vec_name, k_hash_phone, k_vec_phone;
};

static int read_config(const char* argv0, config_params& cfg) {
    const char* config_name = "config.txt";
    char exe_copy[4096];
    strncpy(exe_copy, argv0, sizeof(exe_copy) - 1);
    exe_copy[sizeof(exe_copy) - 1] = '\0';
    char* dir = dirname(exe_copy);

    char config_path[4096];
    snprintf(config_path, sizeof(config_path), "%s/%s", dir, config_name);

    FILE* f = fopen(config_path, "r");
    if (!f) {
        fprintf(stderr, "Cannot open config file: %s\n", config_path);
        return -1;
    }

    int params[4] = {0, 0, 0, 0};
    int idx = 0;
    char line[256];
    while (fgets(line, sizeof(line), f) && idx < 4) {
        const char* p = line;
        while (*p && (*p == ' ' || *p == '\t')) p++;
        if (*p == '#' || *p == '\0' || *p == '\n') continue;
        params[idx++] = atoi(p);
    }
    fclose(f);

    cfg.k_hash_name  = params[0];
    cfg.k_vec_name   = params[1];
    cfg.k_hash_phone = params[2];
    cfg.k_vec_phone  = params[3];

    if (cfg.k_vec_name <= 0) cfg.k_vec_name = 512;
    if (cfg.k_vec_phone <= 0) cfg.k_vec_phone = 512;
    return 0;
}

static int process_select(const command& cmd,
                          name_dv_index& name_idx, phone_dv_index& phone_idx,
                          group_index& grp_idx, record_list& data, strbuf& out) {
    dyn_array<list_node*> found;
    bool use_and = cmd.has_op_land() || cmd.has_op_none();
    int count = 0;

    if (use_and && cmd.has_group_eq()) {
        int gid = cmd.get_group_value();
        if (cmd.has_name_condition()) {
            record pat; pat.init(cmd.get_name_value(), 0, 0);
            dyn_array<list_node*> m;
            grp_idx.find_by_name_in_group(gid, cmd.get_name_cond(), pat, m);
            for (int i = 0; i < m.size(); i++)
                if (cmd.apply(m[i]->rec)) { found.push_back(m[i]); count++; }
        } else if (cmd.has_phone_condition()) {
            record pat; pat.init(nullptr, cmd.get_phone_value(), 0);
            dyn_array<list_node*> m;
            grp_idx.find_by_phone_in_group(gid, cmd.get_phone_cond(), pat, m);
            for (int i = 0; i < m.size(); i++)
                if (cmd.apply(m[i]->rec)) { found.push_back(m[i]); count++; }
        } else {
            dyn_array<list_node*> m;
            grp_idx.collect_group(gid, m);
            for (int i = 0; i < m.size(); i++)
                if (cmd.apply(m[i]->rec)) { found.push_back(m[i]); count++; }
        }
    } else if (use_and && cmd.has_name_condition()) {
        record pat; pat.init(cmd.get_name_value(), 0, 0);
        dyn_array<list_node*> m;
        name_idx.find_by_condition(cmd.get_name_cond(), pat, m);
        for (int i = 0; i < m.size(); i++)
            if (cmd.apply(m[i]->rec)) { found.push_back(m[i]); count++; }
    } else if (use_and && cmd.has_phone_condition()) {
        record pat; pat.init(nullptr, cmd.get_phone_value(), 0);
        dyn_array<list_node*> m;
        phone_idx.find_by_condition(cmd.get_phone_cond(), pat, m);
        for (int i = 0; i < m.size(); i++)
            if (cmd.apply(m[i]->rec)) { found.push_back(m[i]); count++; }
    } else {
        for (list_node* cur = data.head(); cur; cur = cur->next)
            if (cmd.apply(cur->rec)) { found.push_back(cur); count++; }
    }

    sort_found(found, cmd);
    for (int i = 0; i < found.size(); i++)
        print_rec_to_sb(out, found[i]->rec, cmd.get_order());
    sb_write(out, "\n", 1);
    return count;
}

static void process_delete(const command& cmd,
                           name_dv_index& name_idx, phone_dv_index& phone_idx,
                           group_index& grp_idx, record_list& data) {
    dyn_array<list_node*> to_del;
    bool use_and = cmd.has_op_land() || cmd.has_op_none();

    if (use_and && cmd.has_group_eq()) {
        int gid = cmd.get_group_value();
        if (cmd.has_name_condition()) {
            record pat; pat.init(cmd.get_name_value(), 0, 0);
            dyn_array<list_node*> m;
            grp_idx.find_by_name_in_group(gid, cmd.get_name_cond(), pat, m);
            for (int i = 0; i < m.size(); i++)
                if (cmd.apply(m[i]->rec)) to_del.push_back(m[i]);
        } else if (cmd.has_phone_condition()) {
            record pat; pat.init(nullptr, cmd.get_phone_value(), 0);
            dyn_array<list_node*> m;
            grp_idx.find_by_phone_in_group(gid, cmd.get_phone_cond(), pat, m);
            for (int i = 0; i < m.size(); i++)
                if (cmd.apply(m[i]->rec)) to_del.push_back(m[i]);
        } else {
            dyn_array<list_node*> m;
            grp_idx.collect_group(gid, m);
            for (int i = 0; i < m.size(); i++)
                if (cmd.apply(m[i]->rec)) to_del.push_back(m[i]);
        }
    } else if (use_and && cmd.has_name_condition()) {
        record pat; pat.init(cmd.get_name_value(), 0, 0);
        dyn_array<list_node*> m;
        name_idx.find_by_condition(cmd.get_name_cond(), pat, m);
        for (int i = 0; i < m.size(); i++)
            if (cmd.apply(m[i]->rec)) to_del.push_back(m[i]);
    } else if (use_and && cmd.has_phone_condition()) {
        record pat; pat.init(nullptr, cmd.get_phone_value(), 0);
        dyn_array<list_node*> m;
        phone_idx.find_by_condition(cmd.get_phone_cond(), pat, m);
        for (int i = 0; i < m.size(); i++)
            if (cmd.apply(m[i]->rec)) to_del.push_back(m[i]);
    } else {
        list_node* cur = data.head();
        while (cur) {
            list_node* nx = cur->next;
            if (cmd.apply(cur->rec)) to_del.push_back(cur);
            cur = nx;
        }
    }

    for (int i = 0; i < to_del.size(); i++) {
        name_idx.remove_node(to_del[i]);
        phone_idx.remove_node(to_del[i]);
        grp_idx.remove_node(to_del[i]);
        data.remove(to_del[i]);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s filename port\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[2]);

    config_params cfg;
    if (read_config(argv[0], cfg) < 0) return 2;

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("Server: socket"); return 4; }

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Server: bind");
        return 4;
    }
    if (listen(sock, 64) < 0) {
        perror("Server: listen");
        return 4;
    }

    record_list    data;
    name_dv_index  name_idx(cfg.k_vec_name);
    phone_dv_index phone_idx(cfg.k_vec_phone);
    group_index    grp_idx(cfg.k_vec_name, cfg.k_vec_phone);

    FILE* file = fopen(argv[1], "r");
    if (!file) {
        fprintf(stderr, "Cannot open file: %s\n", argv[1]);
        close(sock);
        return 3;
    }

    while (true) {
        record rec;
        io_status status = rec.read(file);
        if (status == io_status::eof) break;
        if (status == io_status::success) {
            list_node* nd = data.push_back(std::move(rec));
            name_idx.add(nd);
        } else {
            fprintf(stderr, "Error reading record\n");
            fclose(file);
            close(sock);
            return 3;
        }
    }
    fclose(file);

    name_idx.finalize(data);
    for (list_node* cur = data.head(); cur; cur = cur->next) {
        phone_idx.add(cur);
        grp_idx.add(cur);
    }
    phone_idx.finalize();
    grp_idx.finalize();

    fd_set active_set;
    FD_ZERO(&active_set);
    FD_SET(sock, &active_set);
    bool stop_server = false;

    strbuf out_sb;
    sb_init(out_sb);

    while (!stop_server) {
        fd_set read_set = active_set;
        if (select(FD_SETSIZE, &read_set, NULL, NULL, NULL) < 0) {
            perror("Server: select");
            break;
        }

        for (int fd = 0; fd < FD_SETSIZE && !stop_server; fd++) {
            if (!FD_ISSET(fd, &read_set)) continue;

            if (fd == sock) {
                struct sockaddr_in client_addr;
                socklen_t sz = sizeof(client_addr);
                int new_fd = accept(sock, (struct sockaddr*)&client_addr, &sz);
                if (new_fd < 0) {
                    perror("Server: accept");
                } else {
                    int flag = 1;
                    setsockopt(new_fd, IPPROTO_TCP, TCP_NODELAY,
                               (const void*)&flag, sizeof(flag));
                    FD_SET(new_fd, &active_set);
                }
                continue;
            }

            char cmd_buf[CMD_BUF_SZ];
            int msg_len = recv_msg(fd, cmd_buf, CMD_BUF_SZ);
            if (msg_len < 0) {
                close(fd);
                FD_CLR(fd, &active_set);
                continue;
            }

            command cmd;
            if (!cmd.parse(cmd_buf)) {
                send_response(fd, 0, nullptr, 0);
                continue;
            }

            if (cmd.get_type() == command_type::quit) {
                send_response(fd, -1, nullptr, 0);
                close(fd);
                FD_CLR(fd, &active_set);
                continue;
            }

            if (cmd.get_type() == command_type::stop) {
                send_response(fd, -1, nullptr, 0);
                close(fd);
                FD_CLR(fd, &active_set);
                stop_server = true;
                continue;
            }

            if (cmd.get_type() == command_type::insert) {
                if (!name_idx.exists(cmd.get_name(), cmd.get_phone(), cmd.get_group())) {
                    record nr;
                    nr.init(cmd.get_name(), cmd.get_phone(), cmd.get_group());
                    list_node* nd = data.push_back(std::move(nr));
                    name_idx.insert_sorted(nd);
                    phone_idx.insert_sorted(nd);
                    grp_idx.insert_sorted(nd);
                }
                send_response(fd, 0, nullptr, 0);
                continue;
            }

            if (cmd.get_type() == command_type::del) {
                process_delete(cmd, name_idx, phone_idx, grp_idx, data);
                send_response(fd, 0, nullptr, 0);
                continue;
            }

            if (cmd.get_type() == command_type::select) {
                out_sb.len = 0;
                int count = process_select(cmd, name_idx, phone_idx, grp_idx, data, out_sb);
                send_response(fd, count,
                              out_sb.buf ? out_sb.buf : "",
                              (int)out_sb.len);
                continue;
            }

            send_response(fd, 0, nullptr, 0);
        }
    }

    for (int fd = 0; fd < FD_SETSIZE; fd++) {
        if (FD_ISSET(fd, &active_set))
            close(fd);
    }

    sb_free(out_sb);

    return 0;
}
