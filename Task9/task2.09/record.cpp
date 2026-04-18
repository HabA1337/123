#include <string.h>
#include <stdio.h>
#include "record.h"

#define LEN 1234

using std::make_unique;

static bool char_in_range(char c, const char*& pattern) {
    if (*pattern != '[') return false;
    
    if (c >= *(pattern + 1) && c <= *(pattern + 3)) {
        return true;
    }
    return false;
}

static bool char_not_in_range(char c, const char*& pattern) {
    if (*pattern != '[') return false;
    if (c < *(pattern + 2) || c > *(pattern + 4)) {
        return true;
    }
    return false;
}

static bool match_pattern(const char* str, const char* pattern) {
    bool escape = false;
    if (!str && !pattern) return true;
    if (!str || !pattern) return false;
    
    while (*pattern) {
        if (!escape && *pattern == '%') {
            pattern++;

            if (!*pattern) return true;

            while (*str) {
                if (match_pattern(str, pattern)) {
                    return true;
                }
                str++;
            }
            return match_pattern(str, pattern);
        }
        else if (!escape && *pattern == '_') {
            if (!*str) return false;
            pattern++;
            str++;
        }
        else if (!escape && *pattern == '[' && *(pattern + 1) && 
                 *(pattern + 2) == '-' && *(pattern + 3) && *(pattern + 4) == ']') {
            if (!*str) return false;
            
            if (!char_in_range(*str, pattern)) {
                return false;
            }
            pattern += 5;
            str++;
        }
        else if (!escape && *pattern == '[' && *(pattern + 1) == '^' && 
                            *(pattern + 2) && *(pattern + 3) && *(pattern + 3) == '-'  &&
                            *(pattern + 4) && *(pattern + 5) == ']') {
            if (!*str) return false;
            if (!char_not_in_range(*str, pattern)) {
                return false;
            }
            pattern += 6;
            str++;
        }
        else if (!escape && *pattern == '\\') {
            escape = true;
            pattern++;
        }
        else {
            escape = false;
            if (*pattern != *str) return false;
            pattern++;
            str++;
        }
    }
    return !escape && !*str;
}

int record::init(const char *n, int p, int g)
{
    phone = p;
    group = g;
    if (n)
    {
        auto n_name = make_unique<char[]>(strlen(n) + 1);
        if (!n_name) return -1;
        strcpy(n_name.get(), n);
        name = std::move(n_name);
    }
    else
        name = nullptr;
    return 0;
}

io_status record::read(FILE *fp)
{
    char buf[LEN];
    name = nullptr;
    if (fscanf(fp, "%s%d%d", buf, &phone, &group) != 3)
    {
        if (feof(fp)) return io_status::eof;
        return io_status::format;
    }
    if (init(buf, phone, group))
        return io_status::memory;
    return io_status::success;
}

void record::print(const ordering order[], FILE *fp) const {
    const int max_items = 3;
    const ordering default_ordering[max_items] = {ordering::name, ordering::phone, ordering::group};
    const ordering* p = (order ? order : default_ordering);
    
    for (int i = 0; i < max_items; i++) {
        switch (p[i]) {
            case ordering::name:
                fprintf(fp, "%s", name.get());
                break;
            case ordering::phone:
                fprintf(fp, "%d", phone);
                break;
            case ordering::group:
                fprintf(fp, "%d", group);
                break;
            case ordering::none:
                continue;
        }
        if (i < max_items - 1 && p[i+1] != ordering::none) {
            fprintf(fp, " ");
        }
    }
    fprintf(fp, "\n");
}

// Check condition 'x' for field 'phone' for 'this' and 'y'
bool record::compare_phone(condition x, const record& y) const
{
    switch (x)
    {
        case condition::none:   // not specified
            return true;        // unspecified operation is true
        case condition::eq:     // equal
            return phone == y.phone;
        case condition::ne:     // not equal
            return phone != y.phone;
        case condition::lt:     // less than
            return phone < y.phone;
        case condition::gt:     // greater than
            return phone > y.phone;
        case condition::le:     // less equal
            return phone <= y.phone;
        case condition::ge:     // great equal
            return phone >= y.phone;
        case condition::like:   // strings only: match pattern
            return false;       // cannot be used for phone
        case condition::nlike:   // strings only: match pattern
            return false;       // cannot be used for phone
    }
    return false;
}

bool record::compare_name(condition x, const record& y) const {
    if (!name.get() || !y.name.get()) {
        if (x == condition::eq) return name == y.name;
        if (x == condition::ne) return name != y.name;
        return false;
    }
    switch (x)
    {
        case condition::none:   // not specified
            return true;        // unspecified operation is true
        case condition::eq:     // equal
            return strcmp(name.get(), y.name.get()) == 0;
        case condition::ne:     // not equal
            return strcmp(name.get(), y.name.get()) != 0;
        case condition::lt:     // less than
            return strcmp(name.get(), y.name.get()) < 0;
        case condition::gt:     // greater than
            return strcmp(name.get(), y.name.get()) > 0;
        case condition::le:     // less equal
            return strcmp(name.get(), y.name.get()) <= 0;
        case condition::ge:     // great equal
            return strcmp(name.get(), y.name.get()) >= 0;
        case condition::like:   // strings only: match pattern
            return match_pattern(name.get(), y.name.get());
        case condition::nlike:
            return !match_pattern(name.get(), y.name.get());
    }
    return false;
}

bool record::compare_group(condition x, const record& y) const {
    switch (x) {
        case condition::none:
            return true;
        case condition::eq:
            return group == y.group;
        case condition::ne:
            return group != y.group;
        case condition::lt:
            return group < y.group;
        case condition::gt:
            return group > y.group;
        case condition::le:
            return group <= y.group;
        case condition::ge:
            return group >= y.group;
        case condition::like:
            return false;
        case condition::nlike:
            return false;
    }
    return false;
}
