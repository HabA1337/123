#include <fstream>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <ctime>
#include <cstdio>
#include "io_status.h"

using entry = std::pair<std::string, int>;

static io_status task1(const char* fa, const char* fb, int& r) {
    std::ifstream in(fa);
    if (!in.is_open()) return io_status::open;

    std::vector<entry> v;
    std::string line;
    int idx = 0;
    while (std::getline(in, line)) {
        v.push_back(std::make_pair(line, idx));
        ++idx;
    }
    in.close();

    std::sort(v.begin(), v.end(),
        [](const entry& x, const entry& y) {
            if (x.first != y.first) return x.first < y.first;
            return x.second < y.second;
        });

    std::vector<entry>::iterator last = std::unique(v.begin(), v.end(),
        [](const entry& x, const entry& y) { return x.first == y.first; });
    v.erase(last, v.end());

    std::sort(v.begin(), v.end(),
        [](const entry& x, const entry& y) { return x.second < y.second; });

    std::ofstream out(fb);
    if (!out.is_open()) return io_status::create;

    for (size_t i = 0; i < v.size(); ++i) {
        out << v[i].first << '\n';
    }
    out.close();

    r = static_cast<int>(v.size());
    return io_status::success;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s fin fout\n", argv[0]);
        return 1;
    }
    int r = 0;
    clock_t start = clock();
    io_status status = task1(argv[1], argv[2], r);
    clock_t end = clock();
    double t = static_cast<double>(end - start) / CLOCKS_PER_SEC;
    printf("%s : Task = %d Result = %d Elapsed = %.2f\n", argv[0], 1, r, t);
    return (status == io_status::success) ? 0 : 1;
}
