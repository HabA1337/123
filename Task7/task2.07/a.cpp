#include <iostream>
#include <cstring>
#include <ctime>
#include <memory>
#include <libgen.h>
#include <fstream>

#include "record.h"
#include "command.h"
#include "record_list.h"
#include "name_index.h"
#include "phone_index.h"

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

    std::unique_ptr<char []> exe_path = std::make_unique<char[]>(strlen(argv0) + 1);
    strcpy(exe_path.get(), argv0);
    char* dir = dirname(exe_path.get());

    size_t path_len = strlen(dir) + 1 + strlen(config_name) + 1;
    std::unique_ptr<char []> config_path = std::make_unique<char[]>(path_len);
    snprintf(config_path.get(), path_len, "%s/%s", dir, config_name);

    std::ifstream config_file(config_path.get());
    if (!config_file.is_open()) {
        std::cerr << "Cannot open config file: " << config_path.get() << std::endl;
        return -1;
    }

    int params[4] = {0, 0, 0, 0};
    int idx = 0;
    std::string line;
    while (std::getline(config_file, line) && idx < 4) {
        const char* p = line.c_str();
        while (*p && isspace(*p)) p++;
        if (*p == '#' || *p == '\0') continue;
        params[idx++] = atoi(p);
    }

    cfg.k_hash_name  = params[0];
    cfg.k_vec_name   = params[1];
    cfg.k_hash_phone = params[2];
    cfg.k_vec_phone  = params[3];

    if (cfg.k_vec_name <= 0) cfg.k_vec_name = 512;
    if (cfg.k_vec_phone <= 0) cfg.k_vec_phone = 512;

    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " filename" << std::endl;
        return 1;
    }

    config_params cfg;
    if (read_config(argv[0], cfg) < 0) return 2;

    record_list     data;
    name_dv_index   name_idx(cfg.k_vec_name);
    phone_dv_index  phone_idx(cfg.k_vec_phone);

    FILE* file = fopen(argv[1], "r");
    if (!file) {
        std::cerr << "Cannot open file: " << argv[1] << std::endl;
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
            std::cerr << "Error reading record" << std::endl;
            fclose(file);
            return 3;
        }
    }
    fclose(file);

    name_idx.finalize(data);

    for (list_node* cur = data.head(); cur; cur = cur->next) {
        phone_idx.add(cur);
    }
    phone_idx.finalize();

    int res = 0;
    std::string input_buffer;
    std::string command_buffer;
    clock_t start = clock();
    bool quit_received = false;

    while (!quit_received && std::getline(std::cin, input_buffer)) {
        command_buffer += input_buffer + " ";
        size_t semicolon_pos = command_buffer.find(';');

        while (!quit_received && semicolon_pos != std::string::npos) {
            std::string full_command = command_buffer.substr(0, semicolon_pos + 1);
            command_buffer.erase(0, semicolon_pos + 1);

            bool is_empty = true;
            for (size_t ci = 0; ci < full_command.size(); ci++) {
                if (!isspace(full_command[ci]) && full_command[ci] != ';') {
                    is_empty = false;
                    break;
                }
            }

            if (!is_empty) {
                command cmd;
                if (!cmd.parse(full_command.c_str())) {
                    std::cerr << "Invalid command: " << full_command << std::endl;
                } else {
                    if (cmd.get_type() == command_type::quit) {
                        quit_received = true;
                        break;
                    }
                    else if (cmd.get_type() == command_type::insert) {
                        if (!name_idx.exists(cmd.get_name(), cmd.get_phone(), cmd.get_group())) {
                            record nr;
                            nr.init(cmd.get_name(), cmd.get_phone(), cmd.get_group());
                            list_node* nd = data.push_back(std::move(nr));
                            name_idx.insert_sorted(nd);
                            phone_idx.insert_sorted(nd);
                        }
                    }
                    else if (cmd.get_type() == command_type::del) {
                        dyn_array<list_node*> to_del;

                        if ((cmd.has_op_land() || cmd.has_op_none()) && cmd.has_name_condition()) {
                            record pat;
                            pat.init(cmd.get_name_value(), 0, 0);
                            dyn_array<list_node*> matches;
                            name_idx.find_by_condition(cmd.get_name_cond(), pat, matches);
                            for (int i = 0; i < matches.size(); i++)
                                if (cmd.apply(matches[i]->rec))
                                    to_del.push_back(matches[i]);
                        }
                        else if ((cmd.has_op_land() || cmd.has_op_none()) && cmd.has_phone_condition()) {
                            record pat;
                            pat.init(nullptr, cmd.get_phone_value(), 0);
                            dyn_array<list_node*> matches;
                            phone_idx.find_by_condition(cmd.get_phone_cond(), pat, matches);
                            for (int i = 0; i < matches.size(); i++)
                                if (cmd.apply(matches[i]->rec))
                                    to_del.push_back(matches[i]);
                        }
                        else {
                            list_node* cur = data.head();
                            while (cur) {
                                list_node* nx = cur->next;
                                if (cmd.apply(cur->rec))
                                    to_del.push_back(cur);
                                cur = nx;
                            }
                        }

                        for (int i = 0; i < to_del.size(); i++) {
                            name_idx.remove_node(to_del[i]);
                            phone_idx.remove_node(to_del[i]);
                            data.remove(to_del[i]);
                        }
                    }
                    else if (cmd.get_type() == command_type::select) {
                        dyn_array<list_node*> found;

                        if ((cmd.has_op_land() || cmd.has_op_none()) && cmd.has_name_condition()) {
                            record pat;
                            pat.init(cmd.get_name_value(), 0, 0);
                            dyn_array<list_node*> matches;
                            name_idx.find_by_condition(cmd.get_name_cond(), pat, matches);
                            for (int i = 0; i < matches.size(); i++) {
                                if (cmd.apply(matches[i]->rec)) {
                                    found.push_back(matches[i]);
                                    res++;
                                }
                            }
                        }
                        else if ((cmd.has_op_land() || cmd.has_op_none()) && cmd.has_phone_condition()) {
                            record pat;
                            pat.init(nullptr, cmd.get_phone_value(), 0);
                            dyn_array<list_node*> matches;
                            phone_idx.find_by_condition(cmd.get_phone_cond(), pat, matches);
                            for (int i = 0; i < matches.size(); i++) {
                                if (cmd.apply(matches[i]->rec)) {
                                    found.push_back(matches[i]);
                                    res++;
                                }
                            }
                        }
                        else {
                            list_node* cur = data.head();
                            while (cur) {
                                if (cmd.apply(cur->rec)) {
                                    found.push_back(cur);
                                    res++;
                                }
                                cur = cur->next;
                            }
                        }

                        sort_found(found, cmd);

                        for (int i = 0; i < found.size(); i++)
                            found[i]->rec.print(cmd.get_order());
                        std::cout << std::endl;
                    }
                }
            }

            if (!quit_received)
                semicolon_pos = command_buffer.find(';');
        }
    }

    clock_t end = clock();
    double elapsed = double(end - start) / CLOCKS_PER_SEC;
    printf("%s : Result = %d Elapsed = %.2f\n", argv[0], res, elapsed);

    return 0;
}
