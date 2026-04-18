#ifndef NAME_INDEX_H
#define NAME_INDEX_H

#include <cstring>
#include <utility>
#include "record.h"
#include "record_list.h"
#include "dyn_array.h"
#include "command.h"

class name_dv_index {
private:
    int k;

    dyn_array<list_node*>* vec;
    int vec_size;
    int vec_cap;

    dyn_array<list_node*> pending;

    void grow_vec() {
        int nc = vec_cap < 4 ? 4 : vec_cap * 2;
        auto* nv = new dyn_array<list_node*>[nc];
        for (int i = 0; i < vec_size; i++)
            nv[i] = std::move(vec[i]);
        delete[] vec;
        vec = nv;
        vec_cap = nc;
    }

    void insert_block(int pos) {
        if (vec_size == vec_cap) grow_vec();
        for (int i = vec_size; i > pos; i--)
            vec[i] = std::move(vec[i - 1]);
        vec[pos] = dyn_array<list_node*>();
        vec_size++;
    }

    void remove_block(int pos) {
        for (int i = pos; i < vec_size - 1; i++)
            vec[i] = std::move(vec[i + 1]);
        vec_size--;
    }

    static const char* nd_name(list_node* nd) {
        return nd->rec.get_name() ? nd->rec.get_name() : "";
    }

    int find_block(const char* name) const {
        if (vec_size == 0) return 0;
        int lo = 0, hi = vec_size - 1, res = 0;
        while (lo <= hi) {
            int mid = lo + (hi - lo) / 2;
            if (vec[mid].size() == 0 || strcmp(nd_name(vec[mid][0]), name) <= 0) {
                res = mid;
                lo = mid + 1;
            } else {
                hi = mid - 1;
            }
        }
        return res;
    }

    int lower_bound_in(const dyn_array<list_node*>& blk, const char* name) const {
        int lo = 0, hi = blk.size();
        while (lo < hi) {
            int mid = lo + (hi - lo) / 2;
            if (strcmp(nd_name(blk[mid]), name) < 0)
                lo = mid + 1;
            else
                hi = mid;
        }
        return lo;
    }

    static int cmp_name(list_node* a, list_node* b) {
        return strcmp(nd_name(a), nd_name(b));
    }

    static int cmp_full(list_node* a, list_node* b) {
        int c = cmp_name(a, b);
        if (c != 0) return c;
        if (a->rec.get_phone() != b->rec.get_phone())
            return a->rec.get_phone() < b->rec.get_phone() ? -1 : 1;
        if (a->rec.get_group() != b->rec.get_group())
            return a->rec.get_group() < b->rec.get_group() ? -1 : 1;
        return 0;
    }

    static void msort(list_node** arr, int n, list_node** buf) {
        if (n <= 1) return;
        int mid = n / 2;
        msort(arr, mid, buf);
        msort(arr + mid, n - mid, buf);
        int i = 0, j = mid, w = 0;
        while (i < mid && j < n) {
            if (cmp_full(arr[i], arr[j]) <= 0)
                buf[w++] = arr[i++];
            else
                buf[w++] = arr[j++];
        }
        while (i < mid) buf[w++] = arr[i++];
        while (j < n)   buf[w++] = arr[j++];
        for (int x = 0; x < n; x++)
            arr[x] = buf[x];
    }

public:
    name_dv_index() : k(1), vec(nullptr), vec_size(0), vec_cap(0) {}

    name_dv_index(int k_val) : k(k_val), vec(nullptr), vec_size(0), vec_cap(0) {
        if (k < 1) k = 1;
    }

    ~name_dv_index() { delete[] vec; }

    name_dv_index(name_dv_index&& o) noexcept
        : k(o.k), vec(o.vec), vec_size(o.vec_size), vec_cap(o.vec_cap),
          pending(std::move(o.pending)) {
        o.vec = nullptr;
        o.vec_size = 0;
        o.vec_cap = 0;
    }

    name_dv_index& operator=(name_dv_index&& o) noexcept {
        if (this != &o) {
            delete[] vec;
            k = o.k;
            vec = o.vec; vec_size = o.vec_size; vec_cap = o.vec_cap;
            pending = std::move(o.pending);
            o.vec = nullptr; o.vec_size = 0; o.vec_cap = 0;
        }
        return *this;
    }

    name_dv_index(const name_dv_index&) = delete;
    name_dv_index& operator=(const name_dv_index&) = delete;

    void add(list_node* nd) {
        pending.push_back(nd);
    }

    void distribute_blocks(int n) {
        int num_blocks = (n + k - 1) / k;
        vec_cap = num_blocks < 4 ? 4 : num_blocks;
        vec = new dyn_array<list_node*>[vec_cap];
        vec_size = num_blocks;

        int pos = 0;
        for (int b = 0; b < num_blocks; b++) {
            int cnt = (b < num_blocks - 1) ? k : (n - pos);
            for (int i = 0; i < cnt; i++)
                vec[b].push_back(pending[pos++]);
        }
        pending.clear();
    }

    void finalize(record_list& lst) {
        int n = pending.size();
        if (n == 0) return;

        list_node** buf = new list_node*[n];
        msort(pending.data(), n, buf);
        delete[] buf;

        int uniq = 1;
        for (int i = 1; i < n; i++) {
            if (cmp_full(pending[uniq - 1], pending[i]) != 0) {
                if (uniq != i) pending[uniq] = pending[i];
                uniq++;
            } else {
                lst.remove(pending[i]);
            }
        }

        distribute_blocks(uniq);
    }

    void finalize() {
        int n = pending.size();
        if (n == 0) return;

        list_node** buf = new list_node*[n];
        msort(pending.data(), n, buf);
        delete[] buf;

        distribute_blocks(n);
    }

    void insert_sorted(list_node* nd) {
        if (vec_size == 0) {
            grow_vec();
            vec[0] = dyn_array<list_node*>();
            vec_size = 1;
        }
        const char* nm = nd_name(nd);
        int bi = find_block(nm);
        int pos = lower_bound_in(vec[bi], nm);
        vec[bi].insert_at(pos, nd);

        if (vec[bi].size() > k) {
            int half = vec[bi].size() / 2;
            insert_block(bi + 1);
            for (int i = half; i < vec[bi].size(); i++)
                vec[bi + 1].push_back(vec[bi][i]);
            vec[bi].resize(half);
        }
    }

    void find_by_condition(condition cond, const record& pattern,
                           dyn_array<list_node*>& result) const {
        if (cond == condition::eq) {
            const char* target = pattern.get_name();
            if (!target) return;
            int bi = find_block(target);
            while (bi > 0 && vec[bi-1].size() > 0 &&
                   strcmp(nd_name(vec[bi-1][vec[bi-1].size()-1]), target) >= 0)
                bi--;
            for (int b = bi; b < vec_size; b++) {
                int start = (b == bi) ? lower_bound_in(vec[b], target) : 0;
                bool found_any = false;
                for (int i = start; i < vec[b].size(); i++) {
                    int c = strcmp(nd_name(vec[b][i]), target);
                    if (c == 0) {
                        result.push_back(vec[b][i]);
                        found_any = true;
                    } else if (c > 0) {
                        return;
                    }
                }
                if (!found_any && b > bi) return;
            }
        } else {
            for (int b = 0; b < vec_size; b++)
                for (int i = 0; i < vec[b].size(); i++)
                    if (vec[b][i]->rec.compare_name(cond, pattern))
                        result.push_back(vec[b][i]);
        }
    }

    bool exists(const char* name, int phone, int grp) const {
        if (!name) return false;
        int bi = find_block(name);
        while (bi > 0 && vec[bi-1].size() > 0 &&
               strcmp(nd_name(vec[bi-1][vec[bi-1].size()-1]), name) >= 0)
            bi--;
        for (int b = bi; b < vec_size; b++) {
            int start = (b == bi) ? lower_bound_in(vec[b], name) : 0;
            for (int i = start; i < vec[b].size(); i++) {
                int c = strcmp(nd_name(vec[b][i]), name);
                if (c > 0) return false;
                if (c == 0 && vec[b][i]->rec.get_phone() == phone &&
                    vec[b][i]->rec.get_group() == grp)
                    return true;
            }
        }
        return false;
    }

    void remove_node(list_node* nd) {
        const char* nm = nd_name(nd);
        int bi = find_block(nm);
        while (bi > 0 && vec[bi-1].size() > 0 &&
               strcmp(nd_name(vec[bi-1][vec[bi-1].size()-1]), nm) >= 0)
            bi--;
        for (int b = bi; b < vec_size; b++) {
            if (vec[b].size() > 0 && strcmp(nd_name(vec[b][0]), nm) > 0) break;
            for (int i = 0; i < vec[b].size(); i++) {
                if (vec[b][i] == nd) {
                    vec[b].erase(i);
                    if (vec[b].empty()) remove_block(b);
                    return;
                }
            }
        }
    }

    void remove_matching(const command& cmd, record_list& lst) {
        for (int b = 0; b < vec_size; ) {
            int w = 0;
            for (int r = 0; r < vec[b].size(); r++) {
                if (!cmd.apply(vec[b][r]->rec)) {
                    if (w != r) vec[b][w] = vec[b][r];
                    w++;
                } else {
                    lst.remove(vec[b][r]);
                }
            }
            vec[b].resize(w);
            if (vec[b].empty()) remove_block(b);
            else b++;
        }
    }

    void remove_matching_by_name(const char* name, const command& cmd,
                                 record_list& lst) {
        int bi = find_block(name);
        for (int b = bi; b < vec_size; ) {
            if (vec[b].size() > 0 && strcmp(nd_name(vec[b][0]), name) > 0) break;
            int w = 0;
            for (int r = 0; r < vec[b].size(); r++) {
                if (!cmd.apply(vec[b][r]->rec)) {
                    if (w != r) vec[b][w] = vec[b][r];
                    w++;
                } else {
                    lst.remove(vec[b][r]);
                }
            }
            vec[b].resize(w);
            if (vec[b].empty()) remove_block(b);
            else b++;
        }
    }

    int num_blocks() const { return vec_size; }
};

#endif
