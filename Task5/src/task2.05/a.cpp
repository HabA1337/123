#include <iostream>
#include <cstring>
#include <ctime>

#include "list_record.h"
#include "command.h"

struct sort_node {
    const record* rec;
    sort_node* next;
};

static sort_node* merge_lists(sort_node* a, sort_node* b, const ordering* ob) {
    sort_node head;
    sort_node* tail = &head;
    head.next = nullptr;
    while (a && b) {
        if (command::compare_records(*a->rec, *b->rec, ob) <= 0) {
            tail->next = a; tail = a; a = a->next;
        } else {
            tail->next = b; tail = b; b = b->next;
        }
    }
    tail->next = a ? a : b;
    return head.next;
}

static sort_node* merge_sort(sort_node* head, const ordering* ob) {
    if (!head || !head->next) return head;
    sort_node* slow = head;
    sort_node* fast = head->next;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
    }
    sort_node* second = slow->next;
    slow->next = nullptr;
    return merge_lists(merge_sort(head, ob), merge_sort(second, ob), ob);
}

void process_select(list_record& records, const command& cmd, int& result_count) {
    const ordering* ob = cmd.get_order_by();
    bool need_sort = (ob[0] != ordering::none);

    sort_node* sort_head = nullptr;
    sort_node* sort_tail = nullptr;
    int found_count = 0;

    for (auto it = records.begin(); it != records.end(); ++it) {
        if (cmd.apply(*it)) {
            found_count++;
            result_count++;
            if (need_sort) {
                sort_node* nd = new sort_node;
                nd->rec = &(*it);
                nd->next = nullptr;
                if (!sort_head) sort_head = sort_tail = nd;
                else { sort_tail->next = nd; sort_tail = nd; }
            } else {
                it->print(cmd.get_order());
            }
        }
    }

    if (need_sort && sort_head) {
        sort_head = merge_sort(sort_head, ob);
        sort_node* cur = sort_head;
        while (cur) {
            cur->rec->print(cmd.get_order());
            sort_node* tmp = cur->next;
            delete cur;
            cur = tmp;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " filename" << std::endl;
        return 1;
    }

    list_record records;
    FILE* file = fopen(argv[1], "r");
    if (!file) {
        std::cerr << "Cannot open file: " << argv[1] << std::endl;
        return 2;
    }

    while (true) {
        record rec;
        io_status status = rec.read(file);
        if (status == io_status::eof) {
            break;
        }
        if (status == io_status::success) {
            records.push_front(std::move(rec));
        } else {
            std::cerr << "Error reading record" << std::endl;
            fclose(file);
            return 3;
        }
    }
    fclose(file);
    
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
            for (char c : full_command) {
                if (!isspace(c) && c != ';') {
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
                        if (!records.exists(cmd.get_name(), cmd.get_phone(), cmd.get_group())) {
                            record new_rec;
                            new_rec.init(cmd.get_name(), cmd.get_phone(), cmd.get_group());
                            records.push_back(std::move(new_rec));
                        }
                    }
                    else if (cmd.get_type() == command_type::del) {
                        auto it = records.begin();
                        while (it != records.end()) {
                            if (cmd.apply(*it)) {
                                it = records.erase(it);
                            } else {
                                ++it;
                            }
                        }
                    }
                    else if (cmd.get_type() == command_type::select) {
                        process_select(records, cmd, res);
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