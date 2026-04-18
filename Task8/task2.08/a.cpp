#include <cstring>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <unistd.h>
#include <libgen.h>

#include "record.h"
#include "command.h"
#include "record_list.h"
#include "name_index.h"
#include "phone_index.h"
#include "group_index.h"

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
    for (int x = 0; x < n; x++)
        arr[x] = buf[x];
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
    int k_hash_name;
    int k_vec_name;
    int k_hash_phone;
    int k_vec_phone;
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

static void process_select(const command& cmd, int& res,
                           name_dv_index& name_idx, phone_dv_index& phone_idx,
                           group_index& grp_idx, record_list& data) {
    dyn_array<list_node*> found;
    bool use_and = cmd.has_op_land() || cmd.has_op_none();

    if (use_and && cmd.has_group_eq()) {
        int gid = cmd.get_group_value();
        if (cmd.has_name_condition()) {
            record pat; pat.init(cmd.get_name_value(), 0, 0);
            dyn_array<list_node*> m;
            grp_idx.find_by_name_in_group(gid, cmd.get_name_cond(), pat, m);
            for (int i = 0; i < m.size(); i++)
                if (cmd.apply(m[i]->rec)) { found.push_back(m[i]); res++; }
        } else if (cmd.has_phone_condition()) {
            record pat; pat.init(nullptr, cmd.get_phone_value(), 0);
            dyn_array<list_node*> m;
            grp_idx.find_by_phone_in_group(gid, cmd.get_phone_cond(), pat, m);
            for (int i = 0; i < m.size(); i++)
                if (cmd.apply(m[i]->rec)) { found.push_back(m[i]); res++; }
        } else {
            dyn_array<list_node*> m;
            grp_idx.collect_group(gid, m);
            for (int i = 0; i < m.size(); i++)
                if (cmd.apply(m[i]->rec)) { found.push_back(m[i]); res++; }
        }
    } else if (use_and && cmd.has_name_condition()) {
        record pat; pat.init(cmd.get_name_value(), 0, 0);
        dyn_array<list_node*> m;
        name_idx.find_by_condition(cmd.get_name_cond(), pat, m);
        for (int i = 0; i < m.size(); i++)
            if (cmd.apply(m[i]->rec)) { found.push_back(m[i]); res++; }
    } else if (use_and && cmd.has_phone_condition()) {
        record pat; pat.init(nullptr, cmd.get_phone_value(), 0);
        dyn_array<list_node*> m;
        phone_idx.find_by_condition(cmd.get_phone_cond(), pat, m);
        for (int i = 0; i < m.size(); i++)
            if (cmd.apply(m[i]->rec)) { found.push_back(m[i]); res++; }
    } else {
        for (list_node* cur = data.head(); cur; cur = cur->next)
            if (cmd.apply(cur->rec)) { found.push_back(cur); res++; }
    }

    sort_found(found, cmd);
    for (int i = 0; i < found.size(); i++)
        found[i]->rec.print(cmd.get_order());
    printf("\n");
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
    if (argc != 2) {
        fprintf(stderr, "Usage: %s filename\n", argv[0]);
        return 1;
    }

    config_params cfg;
    if (read_config(argv[0], cfg) < 0) return 2;

    record_list     data;
    name_dv_index   name_idx(cfg.k_vec_name);
    phone_dv_index  phone_idx(cfg.k_vec_phone);
    group_index     grp_idx(cfg.k_vec_name, cfg.k_vec_phone);

    FILE* file = fopen(argv[1], "r");
    if (!file) {
        fprintf(stderr, "Cannot open file: %s\n", argv[1]);
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

    int res = 0;
    clock_t start = clock();
    bool quit_received = false;

    static const int RBUF_SZ = 4096;
    static const int CMD_SZ  = 65536;
    char rbuf[RBUF_SZ];
    char cmd_buf[CMD_SZ];
    int  cmd_len = 0;

    while (!quit_received) {
        int n = read(STDIN_FILENO, rbuf, RBUF_SZ);
        if (n <= 0) break;

        for (int ri = 0; ri < n && !quit_received; ri++) {
            if (cmd_len < CMD_SZ - 1)
                cmd_buf[cmd_len++] = rbuf[ri];

            if (rbuf[ri] == ';') {
                cmd_buf[cmd_len] = '\0';

                bool is_empty = true;
                for (int j = 0; j < cmd_len; j++) {
                    unsigned char ch = (unsigned char)cmd_buf[j];
                    if (!isspace(ch) && ch != ';') { is_empty = false; break; }
                }

                if (!is_empty) {
                    command cmd;
                    if (cmd.parse(cmd_buf)) {
                        if (cmd.get_type() == command_type::quit) {
                            quit_received = true;
                        } else if (cmd.get_type() == command_type::insert) {
                            if (!name_idx.exists(cmd.get_name(), cmd.get_phone(), cmd.get_group())) {
                                record nr;
                                nr.init(cmd.get_name(), cmd.get_phone(), cmd.get_group());
                                list_node* nd = data.push_back(std::move(nr));
                                name_idx.insert_sorted(nd);
                                phone_idx.insert_sorted(nd);
                                grp_idx.insert_sorted(nd);
                            }
                        } else if (cmd.get_type() == command_type::del) {
                            process_delete(cmd, name_idx, phone_idx, grp_idx, data);
                        } else if (cmd.get_type() == command_type::select) {
                            process_select(cmd, res, name_idx, phone_idx, grp_idx, data);
                        }
                    }
                }
                cmd_len = 0;
            }
        }
    }

    clock_t end = clock();
    double elapsed = double(end - start) / CLOCKS_PER_SEC;
    printf("%s : Result = %d Elapsed = %.2f\n", argv[0], res, elapsed);

    return 0;
}
