#ifndef INDEX_H
#define INDEX_H

#include <cstring>
#include <utility>
#include "record.h"
#include "record_list.h"
#include "dyn_array.h"
#include "command.h"

class name_index {
private:
    int k;
    dyn_array<list_node*>* buckets;

    size_t get_hash(const char* name) const {
        if (!name || name[0] == '\0') return 0;
        size_t hash = 0;
        for (int i = 0; name[i] != '\0'; i++)
            hash += static_cast<size_t>(name[i]);
        return hash % k;
    }

    static int compare_by_name(list_node* a, list_node* b) {
        const char* na = a->rec.get_name() ? a->rec.get_name() : "";
        const char* nb = b->rec.get_name() ? b->rec.get_name() : "";
        return strcmp(na, nb);
    }

    static int compare_full(list_node* a, list_node* b) {
        int cmp = compare_by_name(a, b);
        if (cmp != 0) return cmp;
        if (a->rec.get_phone() != b->rec.get_phone())
            return a->rec.get_phone() < b->rec.get_phone() ? -1 : 1;
        if (a->rec.get_group() != b->rec.get_group())
            return a->rec.get_group() < b->rec.get_group() ? -1 : 1;
        return 0;
    }

    static void merge_impl(list_node** arr, int n, list_node** buf) {
        if (n <= 1) return;
        int mid = n / 2;
        merge_impl(arr, mid, buf);
        merge_impl(arr + mid, n - mid, buf);
        int i = 0, j = mid, w = 0;
        while (i < mid && j < n) {
            if (compare_full(arr[i], arr[j]) <= 0)
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
    name_index(int k_val) : k(k_val) {
        buckets = new dyn_array<list_node*>[k];
    }

    ~name_index() { delete[] buckets; }

    name_index(name_index&& o) noexcept : k(o.k), buckets(o.buckets) {
        o.buckets = nullptr;
        o.k = 0;
    }

    name_index& operator=(name_index&& o) noexcept {
        if (this != &o) {
            delete[] buckets;
            k = o.k;
            buckets = o.buckets;
            o.buckets = nullptr;
            o.k = 0;
        }
        return *this;
    }

    void add(list_node* nd) {
        size_t h = get_hash(nd->rec.get_name());
        buckets[h].push_back(nd);
    }

    void finalize(record_list& lst) {
        int max_sz = 0;
        for (int i = 0; i < k; i++)
            if (buckets[i].size() > max_sz)
                max_sz = buckets[i].size();
        if (max_sz <= 1) return;

        list_node** buf = new list_node*[max_sz];
        for (int i = 0; i < k; i++) {
            int n = buckets[i].size();
            if (n <= 1) continue;
            merge_impl(buckets[i].data(), n, buf);

            int w = 1;
            for (int r = 1; r < n; r++) {
                if (compare_full(buckets[i][w - 1], buckets[i][r]) != 0) {
                    if (w != r)
                        buckets[i][w] = buckets[i][r];
                    w++;
                } else {
                    lst.remove(buckets[i][r]);
                }
            }
            buckets[i].resize(w);
        }
        delete[] buf;
    }

    void insert_sorted(list_node* nd) {
        size_t h = get_hash(nd->rec.get_name());
        dyn_array<list_node*>& bkt = buckets[h];
        const char* rn = nd->rec.get_name() ? nd->rec.get_name() : "";
        int lo = 0, hi = bkt.size() - 1, pos = bkt.size();
        while (lo <= hi) {
            int mid = lo + (hi - lo) / 2;
            const char* mn = bkt[mid]->rec.get_name() ? bkt[mid]->rec.get_name() : "";
            if (strcmp(mn, rn) < 0)
                lo = mid + 1;
            else {
                pos = mid;
                hi = mid - 1;
            }
        }
        bkt.insert_at(pos, nd);
    }

    void find_by_condition(condition cond, const record& pattern,
                           dyn_array<list_node*>& result) const {
        if (cond == condition::eq) {
            const char* target = pattern.get_name();
            if (!target) return;
            size_t h = get_hash(target);
            const dyn_array<list_node*>& bkt = buckets[h];

            int lo = 0, hi = bkt.size() - 1, first = -1;
            while (lo <= hi) {
                int mid = lo + (hi - lo) / 2;
                const char* mn = bkt[mid]->rec.get_name() ? bkt[mid]->rec.get_name() : "";
                int cmp = strcmp(mn, target);
                if (cmp >= 0) {
                    if (cmp == 0) first = mid;
                    hi = mid - 1;
                } else {
                    lo = mid + 1;
                }
            }
            if (first < 0) return;
            for (int i = first; i < bkt.size(); i++) {
                const char* cn = bkt[i]->rec.get_name() ? bkt[i]->rec.get_name() : "";
                if (strcmp(cn, target) != 0) break;
                result.push_back(bkt[i]);
            }
        } else {
            for (int b = 0; b < k; b++) {
                for (int j = 0; j < buckets[b].size(); j++) {
                    if (buckets[b][j]->rec.compare_name(cond, pattern))
                        result.push_back(buckets[b][j]);
                }
            }
        }
    }

    bool exists(const char* name, int phone, int grp) const {
        if (!name) return false;
        size_t h = get_hash(name);
        const dyn_array<list_node*>& bkt = buckets[h];
        for (int i = 0; i < bkt.size(); i++) {
            if (strcmp(bkt[i]->rec.get_name(), name) == 0 &&
                bkt[i]->rec.get_phone() == phone &&
                bkt[i]->rec.get_group() == grp)
                return true;
        }
        return false;
    }

    void remove_node(list_node* nd) {
        size_t h = get_hash(nd->rec.get_name());
        dyn_array<list_node*>& bkt = buckets[h];
        for (int i = 0; i < bkt.size(); i++) {
            if (bkt[i] == nd) {
                bkt.erase(i);
                return;
            }
        }
    }

    void remove_matching(const command& cmd, record_list& lst) {
        for (int i = 0; i < k; i++) {
            int w = 0;
            for (int r = 0; r < buckets[i].size(); r++) {
                if (!cmd.apply(buckets[i][r]->rec)) {
                    if (w != r)
                        buckets[i][w] = buckets[i][r];
                    w++;
                } else {
                    lst.remove(buckets[i][r]);
                }
            }
            buckets[i].resize(w);
        }
    }

    void remove_matching_by_name(const char* name, const command& cmd,
                                 record_list& lst) {
        size_t h = get_hash(name);
        int w = 0;
        for (int r = 0; r < buckets[h].size(); r++) {
            if (!cmd.apply(buckets[h][r]->rec)) {
                if (w != r)
                    buckets[h][w] = buckets[h][r];
                w++;
            } else {
                lst.remove(buckets[h][r]);
            }
        }
        buckets[h].resize(w);
    }

    int num_buckets() const { return k; }
    const dyn_array<list_node*>& bucket(int i) const { return buckets[i]; }
};

#endif
