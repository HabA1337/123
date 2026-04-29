#include <fstream>
#include <string>
#include <list>
#include <algorithm>
#include <ctime>
#include <cstdio>
#include "io_status.h"

static bool is_sep(char c, const std::string& t) {
    return t.find(c) != std::string::npos;
}

static io_status task6(const char* fa, const char* fb, const char* fc,
                       const std::string& t, int& r) {
    std::ifstream ina(fa);
    if (!ina.is_open()) return io_status::open;

    std::list<std::string> words;
    std::string line;
    while (std::getline(ina, line)) {
        std::string w;
        for (size_t i = 0; i <= line.size(); ++i) {
            if (i == line.size() || is_sep(line[i], t)) {
                if (!w.empty()) {
                    words.push_back(w);
                    w.clear();
                }
            } else {
                w += line[i];
            }
        }
    }
    ina.close();

    words.sort();
    std::list<std::string>::iterator last = std::unique(words.begin(), words.end());
    words.erase(last, words.end());

    std::ifstream inb(fb);
    if (!inb.is_open()) return io_status::open;

    std::ofstream out(fc);
    if (!out.is_open()) {
        inb.close();
        return io_status::create;
    }

    int total = 0;
    while (std::getline(inb, line)) {
        int count = 0;
        std::string w;
        for (size_t i = 0; i <= line.size(); ++i) {
            if (i == line.size() || is_sep(line[i], t)) {
                if (!w.empty()) {
                    if (std::find(words.begin(), words.end(), w) != words.end())
                        ++count;
                    w.clear();
                }
            } else {
                w += line[i];
            }
        }
        out << count << ' ' << line << '\n';
        total += count;
    }
    inb.close();
    out.close();

    r = total;
    return io_status::success;
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s fa fb fc t\n", argv[0]);
        return 1;
    }
    int r = 0;
    std::string sep = argv[4];
    clock_t start = clock();
    io_status status = task6(argv[1], argv[2], argv[3], sep, r);
    clock_t end = clock();
    double t = static_cast<double>(end - start) / CLOCKS_PER_SEC;
    printf("%s : Task = %d Result = %d Elapsed = %.2f\n", argv[0], 6, r, t);
    return (status == io_status::success) ? 0 : 1;
}
