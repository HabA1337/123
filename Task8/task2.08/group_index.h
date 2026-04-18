#ifndef GROUP_INDEX_H
#define GROUP_INDEX_H

#include "record_list.h"
#include "name_index.h"
#include "phone_index.h"
#include "command.h"
#include "dyn_array.h"

class group_index {
private:
    struct group_entry {
        int group_id;
        name_dv_index  name_idx;
        phone_dv_index phone_idx;
        int count;

        group_entry() : group_id(-1), count(0) {}
    };

    dyn_array<group_entry> table;
    int k_name;
    int k_phone;

    int find_pos(int gid) const {
        int lo = 0, hi = table.size() - 1;
        while (lo <= hi) {
            int mid = lo + (hi - lo) / 2;
            if (table[mid].group_id == gid) return mid;
            if (table[mid].group_id < gid) lo = mid + 1;
            else hi = mid - 1;
        }
        return -(lo + 1);
    }

public:
    group_index(int kn, int kp) : k_name(kn), k_phone(kp) {}

    group_index(const group_index&) = delete;
    group_index& operator=(const group_index&) = delete;

    group_entry* find(int gid) {
        int p = find_pos(gid);
        return p >= 0 ? &table[p] : nullptr;
    }

    const group_entry* find(int gid) const {
        int p = find_pos(gid);
        return p >= 0 ? &table[p] : nullptr;
    }

    group_entry* find_or_create(int gid) {
        int p = find_pos(gid);
        if (p >= 0) return &table[p];

        int ins = -(p + 1);
        group_entry ge;
        ge.group_id = gid;
        ge.name_idx = name_dv_index(k_name);
        ge.phone_idx = phone_dv_index(k_phone);
        ge.count = 0;
        table.insert_at(ins, std::move(ge));
        return &table[ins];
    }

    void add(list_node* nd) {
        group_entry* ge = find_or_create(nd->rec.get_group());
        ge->name_idx.add(nd);
        ge->phone_idx.add(nd);
        ge->count++;
    }

    void finalize() {
        for (int i = 0; i < table.size(); i++) {
            table[i].name_idx.finalize();
            table[i].phone_idx.finalize();
        }
    }

    void insert_sorted(list_node* nd) {
        group_entry* ge = find_or_create(nd->rec.get_group());
        ge->name_idx.insert_sorted(nd);
        ge->phone_idx.insert_sorted(nd);
        ge->count++;
    }

    void remove_node(list_node* nd) {
        group_entry* ge = find(nd->rec.get_group());
        if (!ge) return;
        ge->name_idx.remove_node(nd);
        ge->phone_idx.remove_node(nd);
        ge->count--;
    }

    void find_by_name_in_group(int gid, condition cond, const record& pattern,
                               dyn_array<list_node*>& result) const {
        const group_entry* ge = find(gid);
        if (!ge) return;
        ge->name_idx.find_by_condition(cond, pattern, result);
    }

    void find_by_phone_in_group(int gid, condition cond, const record& pattern,
                                dyn_array<list_node*>& result) const {
        const group_entry* ge = find(gid);
        if (!ge) return;
        ge->phone_idx.find_by_condition(cond, pattern, result);
    }

    void collect_group(int gid, dyn_array<list_node*>& result) const {
        const group_entry* ge = find(gid);
        if (!ge) return;
        record pat;
        pat.init(nullptr, 0, 0);
        ge->name_idx.find_by_condition(condition::none, pat, result);
    }

    int num_groups() const { return table.size(); }
};

#endif
