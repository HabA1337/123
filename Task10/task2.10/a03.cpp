#include <fstream>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <ctime>
#include <cstdio>
#include "io_status.h"

using entry = std::pair<std::string, std::pair<int, int>>;

static io_status task3(const char* fa, const char* fb, int& r) {
    std::ifstream in(fa);
    if (!in.is_open()) return io_status::open;

    std::vector<entry> v;
    std::string line;
    int idx = 1;
    while (std::getline(in, line)) {
        v.push_back(std::make_pair(line, std::make_pair(idx, 0)));
        ++idx;
    }
    in.close();

    std::sort(v.begin(), v.end(),
        [](const entry& x, const entry& y) { return x.first < y.first; });

    int unique_count = 0;
    for (size_t i = 0; i < v.size(); ) {
        size_t j = i;
        while (j < v.size() && v[j].first == v[i].first) ++j;
        int cnt = static_cast<int>(j - i);
        for (size_t k = i; k < j; ++k) {
            v[k].second.second = cnt;
        }
        ++unique_count;
        i = j;
    }
    r = unique_count;

    std::sort(v.begin(), v.end(),
        [](const entry& x, const entry& y) {
            return x.second.first < y.second.first;
        });

    std::ofstream out(fb);
    if (!out.is_open()) return io_status::create;

    for (size_t i = 0; i < v.size(); ++i) {
        out << v[i].second.first << ' '
            << v[i].second.second << ' '
            << v[i].first << '\n';
    }
    out.close();

    return io_status::success;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s fin fout\n", argv[0]);
        return 1;
    }
    int r = 0;
    clock_t start = clock();
    io_status status = task3(argv[1], argv[2], r);
    clock_t end = clock();
    double t = static_cast<double>(end - start) / CLOCKS_PER_SEC;
    printf("%s : Task = %d Result = %d Elapsed = %.2f\n", argv[0], 3, r, t);
    return (status == io_status::success) ? 0 : 1;
}
