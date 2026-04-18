#ifndef PHONE_INDEX_H
#define PHONE_INDEX_H

#include <cstring>
#include <utility>
#include "record.h"
#include "record_list.h"
#include "dyn_array.h"
#include "command.h"

class phone_dv_index {
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

    static int cmp_phone(list_node* a, list_node* b) {
        if (a->rec.get_phone() != b->rec.get_phone())
            return a->rec.get_phone() < b->rec.get_phone() ? -1 : 1;
        return 0;
    }

    static int cmp_full(list_node* a, list_node* b) {
        int c = cmp_phone(a, b);
        if (c != 0) return c;
        const char* na = a->rec.get_name() ? a->rec.get_name() : "";
        const char* nb = b->rec.get_name() ? b->rec.get_name() : "";
        c = strcmp(na, nb);
        if (c != 0) return c;
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

    int find_block(int phone) const {
        if (vec_size == 0) return 0;
        int lo = 0, hi = vec_size - 1, res = 0;
        while (lo <= hi) {
            int mid = lo + (hi - lo) / 2;
            if (vec[mid].size() == 0 || vec[mid][0]->rec.get_phone() <= phone) {
                res = mid;
                lo = mid + 1;
            } else {
                hi = mid - 1;
            }
        }
        return res;
    }

    int lower_bound_in(const dyn_array<list_node*>& blk, int phone) const {
        int lo = 0, hi = blk.size();
        while (lo < hi) {
            int mid = lo + (hi - lo) / 2;
            if (blk[mid]->rec.get_phone() < phone)
                lo = mid + 1;
            else
                hi = mid;
        }
        return lo;
    }

public:
    phone_dv_index() : k(1), vec(nullptr), vec_size(0), vec_cap(0) {}

    phone_dv_index(int k_val) : k(k_val), vec(nullptr), vec_size(0), vec_cap(0) {
        if (k < 1) k = 1;
    }

    ~phone_dv_index() { delete[] vec; }

    phone_dv_index(phone_dv_index&& o) noexcept
        : k(o.k), vec(o.vec), vec_size(o.vec_size), vec_cap(o.vec_cap),
          pending(std::move(o.pending)) {
        o.vec = nullptr;
        o.vec_size = 0;
        o.vec_cap = 0;
    }

    phone_dv_index& operator=(phone_dv_index&& o) noexcept {
        if (this != &o) {
            delete[] vec;
            k = o.k;
            vec = o.vec; vec_size = o.vec_size; vec_cap = o.vec_cap;
            pending = std::move(o.pending);
            o.vec = nullptr; o.vec_size = 0; o.vec_cap = 0;
        }
        return *this;
    }

    phone_dv_index(const phone_dv_index&) = delete;
    phone_dv_index& operator=(const phone_dv_index&) = delete;

    void add(list_node* nd) {
        pending.push_back(nd);
    }

    void finalize() {
        int n = pending.size();
        if (n == 0) return;

        list_node** buf = new list_node*[n];
        msort(pending.data(), n, buf);
        delete[] buf;

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

    void insert_sorted(list_node* nd) {
        if (vec_size == 0) {
            grow_vec();
            vec[0] = dyn_array<list_node*>();
            vec_size = 1;
        }
        int ph = nd->rec.get_phone();
        int bi = find_block(ph);
        int pos = lower_bound_in(vec[bi], ph);
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
            int target = pattern.get_phone();
            int bi = find_block(target);
            while (bi > 0 && vec[bi-1].size() > 0 &&
                   vec[bi-1][vec[bi-1].size()-1]->rec.get_phone() >= target)
                bi--;
            for (int b = bi; b < vec_size; b++) {
                int start = (b == bi) ? lower_bound_in(vec[b], target) : 0;
                bool found_any = false;
                for (int i = start; i < vec[b].size(); i++) {
                    int ph = vec[b][i]->rec.get_phone();
                    if (ph == target) {
                        result.push_back(vec[b][i]);
                        found_any = true;
                    } else if (ph > target) {
                        return;
                    }
                }
                if (!found_any && b > bi) return;
            }
        } else {
            for (int b = 0; b < vec_size; b++)
                for (int i = 0; i < vec[b].size(); i++)
                    if (vec[b][i]->rec.compare_phone(cond, pattern))
                        result.push_back(vec[b][i]);
        }
    }

    bool exists(const char* name, int phone, int grp) const {
        int bi = find_block(phone);
        while (bi > 0 && vec[bi-1].size() > 0 &&
               vec[bi-1][vec[bi-1].size()-1]->rec.get_phone() >= phone)
            bi--;
        for (int b = bi; b < vec_size; b++) {
            int start = (b == bi) ? lower_bound_in(vec[b], phone) : 0;
            for (int i = start; i < vec[b].size(); i++) {
                int ph = vec[b][i]->rec.get_phone();
                if (ph > phone) return false;
                if (ph == phone) {
                    const char* cn = vec[b][i]->rec.get_name();
                    if (cn && name && strcmp(cn, name) == 0 &&
                        vec[b][i]->rec.get_group() == grp)
                        return true;
                }
            }
        }
        return false;
    }

    void remove_node(list_node* nd) {
        int ph = nd->rec.get_phone();
        int bi = find_block(ph);
        while (bi > 0 && vec[bi-1].size() > 0 &&
               vec[bi-1][vec[bi-1].size()-1]->rec.get_phone() >= ph)
            bi--;
        for (int b = bi; b < vec_size; b++) {
            if (vec[b].size() > 0 && vec[b][0]->rec.get_phone() > ph) break;
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

    int num_blocks() const { return vec_size; }
};

#endif
