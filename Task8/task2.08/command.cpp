#include "command.h"
#include <cstring>
#include <cctype>
#include <cstdlib>

#define MAX_BUF 8192

void command::skip_space(const char** str) {
    while (**str && isspace((unsigned char)**str)) (*str)++;
}

void command::reset() {
    type = command_type::none;
    c_name = condition::none;
    c_phone = condition::none;
    c_group = condition::none;
    op = operation::none;
    for (int i = 0; i < max_items; i++) {
        order[i] = ordering::none;
        order_by[i] = ordering::none;
    }
}

bool command::parse(const char* string) {
    reset();
    const char* s = string;
    skip_space(&s);

    if (strncmp(s, "quit", 4) == 0) { s += 4; return parse_quit(s); }
    if (strncmp(s, "insert", 6) == 0) { s += 6; return parse_insert(s); }
    if (strncmp(s, "select", 6) == 0) { s += 6; return parse_select(s); }
    if (strncmp(s, "delete", 6) == 0) { s += 6; return parse_delete(s); }
    return false;
}

bool command::parse_quit(const char* string) {
    const char* s = string;
    skip_space(&s);
    if (*s == ';') { type = command_type::quit; return true; }
    return false;
}

bool command::parse_insert(const char* string) {
    char name_buf[1024];
    const char* s = string;
    skip_space(&s);
    if (*s != '(') return false;
    s++;
    skip_space(&s);

    const char* name_start = s;
    while (*s && *s != ',') s++;
    if (*s != ',') return false;

    int name_len = (int)(s - name_start);
    if (name_len >= (int)sizeof(name_buf)) return false;
    memcpy(name_buf, name_start, name_len);
    name_buf[name_len] = '\0';
    char* e = name_buf + name_len - 1;
    while (e >= name_buf && isspace((unsigned char)*e)) { *e = '\0'; e--; }

    s++;
    skip_space(&s);
    char* endptr;
    long phone_val = strtol(s, &endptr, 10);
    if (endptr == s) return false;
    s = endptr;
    skip_space(&s);
    if (*s != ',') return false;
    s++;
    skip_space(&s);
    long group_val = strtol(s, &endptr, 10);
    if (endptr == s) return false;
    s = endptr;
    skip_space(&s);
    if (*s != ')') return false;
    s++;
    skip_space(&s);
    if (*s != ';') return false;

    type = command_type::insert;
    init(name_buf, (int)phone_val, (int)group_val);
    return true;
}

bool command::parse_select(const char* string) {
    type = command_type::select;
    const char* s = string;
    skip_space(&s);

    const char* f_start = s;
    while (*s && *s != ';' &&
           strncmp(s, "where", 5) != 0 &&
           strncmp(s, "order by", 8) != 0)
        s++;
    int flen = (int)(s - f_start);
    char fields_buf[MAX_BUF];
    if (flen >= MAX_BUF) return false;
    memcpy(fields_buf, f_start, flen);
    fields_buf[flen] = '\0';
    if (!parse_output_fields(fields_buf)) return false;

    skip_space(&s);
    if (strncmp(s, "where", 5) == 0) {
        s += 5;
        skip_space(&s);
        const char* w_start = s;
        while (*s && *s != ';' && strncmp(s, "order by", 8) != 0) s++;
        int wlen = (int)(s - w_start);
        char where_buf[MAX_BUF];
        if (wlen >= MAX_BUF) return false;
        memcpy(where_buf, w_start, wlen);
        where_buf[wlen] = '\0';
        if (!parse_search_condition(where_buf)) return false;
        skip_space(&s);
    }

    if (strncmp(s, "order by", 8) == 0) {
        s += 8;
        skip_space(&s);
        const char* o_start = s;
        while (*s && *s != ';') s++;
        int olen = (int)(s - o_start);
        char order_buf[MAX_BUF];
        if (olen >= MAX_BUF) return false;
        memcpy(order_buf, o_start, olen);
        order_buf[olen] = '\0';
        if (!parse_order_by(order_buf)) return false;
        skip_space(&s);
    }

    return *s == ';';
}

bool command::parse_delete(const char* string) {
    type = command_type::del;
    const char* s = string;
    skip_space(&s);
    if (*s == ';') return true;

    if (strncmp(s, "where", 5) == 0) {
        s += 5;
        skip_space(&s);
        const char* w_start = s;
        while (*s && *s != ';') s++;
        int wlen = (int)(s - w_start);
        char where_buf[MAX_BUF];
        if (wlen >= MAX_BUF) return false;
        memcpy(where_buf, w_start, wlen);
        where_buf[wlen] = '\0';
        if (!parse_search_condition(where_buf)) return false;
        skip_space(&s);
    }

    return *s == ';';
}

bool command::parse_output_fields(const char* fields) {
    while (isspace((unsigned char)*fields)) fields++;
    if (!*fields) return false;

    int len = (int)strlen(fields);
    char buf[MAX_BUF];
    if (len >= MAX_BUF) return false;
    strcpy(buf, fields);

    char* e = buf + len - 1;
    while (e > buf && isspace((unsigned char)*e)) { *e = '\0'; e--; }

    if (strcmp(buf, "*") == 0) {
        order[0] = ordering::name;
        order[1] = ordering::phone;
        order[2] = ordering::group;
        return true;
    }

    char* token = strtok(buf, ",");
    int idx = 0;
    while (token && idx < 3) {
        while (isspace((unsigned char)*token)) token++;
        char* te = token + strlen(token) - 1;
        while (te > token && isspace((unsigned char)*te)) { *te = '\0'; te--; }

        ordering ord = ordering::none;
        if (strcmp(token, "name") == 0) ord = ordering::name;
        else if (strcmp(token, "phone") == 0) ord = ordering::phone;
        else if (strcmp(token, "group") == 0) ord = ordering::group;
        else return false;

        for (int i = 0; i < idx; i++)
            if (order[i] == ord) return false;
        order[idx++] = ord;
        token = strtok(nullptr, ",");
    }
    return token == nullptr;
}

bool command::parse_order_by(const char* fields) {
    while (isspace((unsigned char)*fields)) fields++;
    if (!*fields) return false;

    int len = (int)strlen(fields);
    char buf[MAX_BUF];
    if (len >= MAX_BUF) return false;
    strcpy(buf, fields);

    char* token = strtok(buf, ",");
    int idx = 0;
    while (token && idx < max_items) {
        while (isspace((unsigned char)*token)) token++;
        char* te = token + strlen(token) - 1;
        while (te > token && isspace((unsigned char)*te)) { *te = '\0'; te--; }

        ordering ord = ordering::none;
        if (strcmp(token, "name") == 0) ord = ordering::name;
        else if (strcmp(token, "phone") == 0) ord = ordering::phone;
        else if (strcmp(token, "group") == 0) ord = ordering::group;
        else return false;

        for (int i = 0; i < idx; i++)
            if (order_by[i] == ord) return false;
        order_by[idx++] = ord;
        token = strtok(nullptr, ",");
    }
    return token == nullptr;
}

bool command::parse_search_condition(const char* cond_str) {
    if (!cond_str || !*cond_str) return false;

    const char* and_pos = strstr(cond_str, " and ");
    const char* or_pos  = strstr(cond_str, " or ");

    if (and_pos && (!or_pos || and_pos < or_pos)) {
        op = operation::land;
        int len1 = (int)(and_pos - cond_str);
        char c1[MAX_BUF];
        if (len1 >= MAX_BUF) return false;
        memcpy(c1, cond_str, len1);
        c1[len1] = '\0';
        return parse_single_condition(c1) &&
               parse_search_condition(and_pos + 5);
    }
    if (or_pos && (!and_pos || or_pos < and_pos)) {
        op = operation::lor;
        int len1 = (int)(or_pos - cond_str);
        char c1[MAX_BUF];
        if (len1 >= MAX_BUF) return false;
        memcpy(c1, cond_str, len1);
        c1[len1] = '\0';
        return parse_single_condition(c1) &&
               parse_search_condition(or_pos + 4);
    }
    return parse_single_condition(cond_str);
}

bool command::parse_single_condition(const char* string) {
    while (isspace((unsigned char)*string)) string++;
    if (!*string) return false;

    int slen = (int)strlen(string);
    char buf[MAX_BUF];
    if (slen >= MAX_BUF) return false;
    strcpy(buf, string);

    char* field = strtok(buf, " \t");
    if (!field) return false;

    char* op_str = strtok(nullptr, " \t");
    if (!op_str) return false;

    condition cond;
    if (strcmp(op_str, "not") == 0) {
        char* like_str = strtok(nullptr, " \t");
        if (!like_str || strcmp(like_str, "like") != 0) return false;
        cond = condition::nlike;
    } else if (strcmp(op_str, "like") == 0) {
        cond = condition::like;
    } else if (strcmp(op_str, "=") == 0) {
        cond = condition::eq;
    } else if (strcmp(op_str, "<>") == 0) {
        cond = condition::ne;
    } else if (strcmp(op_str, "<") == 0) {
        cond = condition::lt;
    } else if (strcmp(op_str, ">") == 0) {
        cond = condition::gt;
    } else if (strcmp(op_str, "<=") == 0) {
        cond = condition::le;
    } else if (strcmp(op_str, ">=") == 0) {
        cond = condition::ge;
    } else {
        return false;
    }

    char* value = strtok(nullptr, "");
    if (!value) return false;
    while (isspace((unsigned char)*value)) value++;
    if (!*value) return false;
    char* ve = value + strlen(value) - 1;
    while (ve > value && isspace((unsigned char)*ve)) { *ve = '\0'; ve--; }

    if (strcmp(field, "name") == 0) {
        if (c_name != condition::none) return false;
        c_name = cond;
        init(value, get_phone(), get_group());
        return true;
    }
    if (strcmp(field, "phone") == 0) {
        if (c_phone != condition::none) return false;
        if (cond == condition::like || cond == condition::nlike) return false;
        c_phone = cond;
        char* endptr;
        long val = strtol(value, &endptr, 10);
        if (*endptr) return false;
        init(get_name(), (int)val, get_group());
        return true;
    }
    if (strcmp(field, "group") == 0) {
        if (c_group != condition::none) return false;
        if (cond == condition::like || cond == condition::nlike) return false;
        c_group = cond;
        char* endptr;
        long val = strtol(value, &endptr, 10);
        if (*endptr) return false;
        init(get_name(), get_phone(), (int)val);
        return true;
    }
    return false;
}

bool command::apply(const record& x) const {
    bool def_val = (op == operation::land);

    bool name_res  = (c_name  == condition::none) ? def_val : x.compare_name(c_name, *this);
    bool phone_res = (c_phone == condition::none) ? def_val : x.compare_phone(c_phone, *this);
    bool group_res = (c_group == condition::none) ? def_val : x.compare_group(c_group, *this);

    if (op == operation::land) return name_res && phone_res && group_res;
    if (op == operation::lor)  return name_res || phone_res || group_res;

    if (c_name  != condition::none) return name_res;
    if (c_phone != condition::none) return phone_res;
    if (c_group != condition::none) return group_res;
    return true;
}

int command::compare_records(const record& a, const record& b, const ordering ob[]) {
    for (int i = 0; i < 3; i++) {
        if (ob[i] == ordering::none) break;
        int cmp = 0;
        switch (ob[i]) {
            case ordering::name: {
                const char* na = a.get_name() ? a.get_name() : "";
                const char* nb = b.get_name() ? b.get_name() : "";
                cmp = strcmp(na, nb);
                break;
            }
            case ordering::phone:
                if (a.get_phone() != b.get_phone())
                    cmp = a.get_phone() < b.get_phone() ? -1 : 1;
                break;
            case ordering::group:
                if (a.get_group() != b.get_group())
                    cmp = a.get_group() < b.get_group() ? -1 : 1;
                break;
            default: break;
        }
        if (cmp != 0) return cmp;
    }
    return 0;
}

void command::print(FILE* fp) const {
    fprintf(fp, "Command type: ");
    switch (type) {
        case command_type::quit: fprintf(fp, "QUIT\n"); return;
        case command_type::insert:
            fprintf(fp, "INSERT (%s, %d, %d)\n",
                    get_name() ? get_name() : "null", get_phone(), get_group());
            return;
        case command_type::select: fprintf(fp, "SELECT\n"); break;
        case command_type::del:    fprintf(fp, "DELETE\n"); break;
        default: fprintf(fp, "NONE\n"); return;
    }

    if (type == command_type::select) {
        fprintf(fp, "  Output fields:");
        for (int i = 0; i < 3; i++) {
            if (order[i] == ordering::none) continue;
            switch (order[i]) {
                case ordering::name:  fprintf(fp, " name"); break;
                case ordering::phone: fprintf(fp, " phone"); break;
                case ordering::group: fprintf(fp, " group"); break;
                default: break;
            }
        }
        fprintf(fp, "\n");
    }

    if (c_name != condition::none || c_phone != condition::none || c_group != condition::none) {
        fprintf(fp, "  Conditions:");
        if (c_name  != condition::none) fprintf(fp, " name");
        if (c_phone != condition::none) fprintf(fp, " phone");
        if (c_group != condition::none) fprintf(fp, " group");
        if (op == operation::land) fprintf(fp, " (AND)");
        if (op == operation::lor)  fprintf(fp, " (OR)");
        fprintf(fp, "\n");
    }

    if (type == command_type::select) {
        bool first = true;
        for (int i = 0; i < 3; i++) {
            if (order_by[i] != ordering::none) {
                if (first) { fprintf(fp, "  Order by:"); first = false; }
                switch (order_by[i]) {
                    case ordering::name:  fprintf(fp, " name"); break;
                    case ordering::phone: fprintf(fp, " phone"); break;
                    case ordering::group: fprintf(fp, " group"); break;
                    default: break;
                }
            }
        }
        if (!first) fprintf(fp, "\n");
    }
}
