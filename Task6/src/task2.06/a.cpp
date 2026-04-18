#include <iostream>
#include <cstring>
#include <ctime>
#include <memory>
#include <libgen.h>
#include <fstream>

#include "record.h"
#include "command.h"
#include "record_list.h"
#include "index.h"

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

int read_config(const char* argv0) {
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

    int k_val = 0;
    std::string line;
    while (std::getline(config_file, line)) {
        const char* p = line.c_str();
        while (*p && isspace(*p)) p++;
        if (*p == '#' || *p == '\0') continue;

        if (k_val == 0) {
            k_val = atoi(p);
        }
    }

    return k_val;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " filename" << std::endl;
        return 1;
    }

    int k = read_config(argv[0]);

    if (k <= 0) {
        std::cerr << "The parameter k cannot be less than zero" << std::endl;
        return 2;
    }

    record_list data;
    name_index  index(k);

    FILE* file = fopen(argv[1], "r");
    if (!file) {
        std::cerr << "Cannot open file: " << argv[1] << std::endl;
        return 3;
    }

    while (true) {
        record rec;
        io_status status = rec.read(file);
        if (status == io_status::eof) {
            break;
        }
        if (status == io_status::success) {
            list_node* nd = data.push_back(std::move(rec));
            index.add(nd);
        } else {
            std::cerr << "Error reading record" << std::endl;
            fclose(file);
            return 3;
        }
    }
    fclose(file);

    index.finalize(data);

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
                        if (!index.exists(cmd.get_name(), cmd.get_phone(), cmd.get_group())) {
                            record nr;
                            nr.init(cmd.get_name(), cmd.get_phone(), cmd.get_group());
                            list_node* nd = data.push_back(std::move(nr));
                            index.insert_sorted(nd);
                        }
                    }
                    else if (cmd.get_type() == command_type::del) {
                        if (cmd.has_name_eq() && (cmd.has_op_land() || cmd.has_op_none())) {
                            index.remove_matching_by_name(cmd.get_name_value(), cmd, data);
                        } else {
                            index.remove_matching(cmd, data);
                        }
                    }
                    else if (cmd.get_type() == command_type::select) {
                        dyn_array<list_node*> found;

                        if ((cmd.has_op_land() || cmd.has_op_none()) && cmd.has_name_condition()) {
                            record pattern;
                            pattern.init(cmd.get_name_value(), 0, 0);
                            dyn_array<list_node*> name_matches;
                            index.find_by_condition(cmd.get_name_cond(), pattern, name_matches);

                            for (int i = 0; i < name_matches.size(); i++) {
                                if (cmd.apply(name_matches[i]->rec)) {
                                    found.push_back(name_matches[i]);
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

                        for (int i = 0; i < found.size(); i++) {
                            found[i]->rec.print(cmd.get_order());
                        }
                        std::cout << std::endl;
                    }
                }
            }

            if (!quit_received) {
                semicolon_pos = command_buffer.find(';');
            }
        }
    }

    clock_t end = clock();
    double elapsed = double(end - start) / CLOCKS_PER_SEC;
    printf("%s : Result = %d Elapsed = %.2f\n", argv[0], res, elapsed);

    return 0;
}
