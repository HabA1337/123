#include <fstream>
#include <string>
#include <list>
#include <utility>
#include <algorithm>
#include <ctime>
#include <cstdio>
#include "io_status.h"

using entry = std::pair<std::string, std::pair<int, int>>;

static io_status task4(const char* fa, const char* fb, int& r) {
    std::ifstream in(fa);
    if (!in.is_open()) return io_status::open;

    std::list<entry> l;
    std::string line;
    int idx = 1;
    while (std::getline(in, line)) {
        l.push_back(std::make_pair(line, std::make_pair(idx, 0)));
        ++idx;
    }
    in.close();

    l.sort([](const entry& x, const entry& y) { return x.first < y.first; });

    int unique_count = 0;
    std::list<entry>::iterator it = l.begin();
    while (it != l.end()) {
        std::list<entry>::iterator group_start = it;
        int cnt = 0;
        while (it != l.end() && it->first == group_start->first) {
            ++cnt;
            ++it;
        }
        for (std::list<entry>::iterator k = group_start; k != it; ++k) {
            k->second.second = cnt;
        }
        ++unique_count;
    }
    r = unique_count;

    l.sort([](const entry& x, const entry& y) {
        return x.second.first < y.second.first;
    });

    std::ofstream out(fb);
    if (!out.is_open()) return io_status::create;

    for (std::list<entry>::const_iterator i2 = l.begin(); i2 != l.end(); ++i2) {
        out << i2->second.first << ' '
            << i2->second.second << ' '
            << i2->first << '\n';
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
    io_status status = task4(argv[1], argv[2], r);
    clock_t end = clock();
    double t = static_cast<double>(end - start) / CLOCKS_PER_SEC;
    printf("%s : Task = %d Result = %d Elapsed = %.2f\n", argv[0], 4, r, t);
    return (status == io_status::success) ? 0 : 1;
}
