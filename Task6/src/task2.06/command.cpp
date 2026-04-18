#include "command.h"
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <memory>

static condition str_to_condition(const char* op) {
    if (strcmp(op, "=") == 0) return condition::eq;
    if (strcmp(op, "<>") == 0) return condition::ne;
    if (strcmp(op, "<") == 0) return condition::lt;
    if (strcmp(op, ">") == 0) return condition::gt;
    if (strcmp(op, "<=") == 0) return condition::le;
    if (strcmp(op, ">=") == 0) return condition::ge;
    if (strcmp(op, "like") == 0) return condition::like;
    return condition::none;
}

void command::skip_space(const char** str) {
    while (**str && (isspace(**str) || **str == '\n')) (*str)++;
}

bool command::parse(const char* string) {
    type = command_type::none;
    c_name = condition::none;
    c_phone = condition::none;
    c_group = condition::none;
    op = operation::none;
    for (int i = 0; i < max_items; i++) {
        order[i] = ordering::none;
        order_by[i] = ordering::none;
    }
    
    const char* s = string;
    skip_space(&s);
    
    if (strncmp(s, "quit", 4) == 0) {
        s += strlen("quit");
        return parse_quit(s);
    }
    else if (strncmp(s, "insert", 6) == 0) {
        s += strlen("insert");
        return parse_insert(s);
    }
    else if (strncmp(s, "select", 6) == 0) {
        s += strlen("select");;
        return parse_select(s);
    }
    else if (strncmp(s, "delete", 6) == 0) {
        s += strlen("delete");;
        return parse_delete(s);
    }
    
    return false;
}

bool command::parse_quit(const char* string) {
    const char* s = string;
    skip_space(&s);
    
    if (*s == ';') {
        type = command_type::quit;
        return true;
    }
    return false;
}

bool command::parse_insert(const char* string) {
    char name_buf[1024];
    
    const char* s = string;
    skip_space(&s);

    if (*s != '(') {
        return false;
    }
    s++;
    skip_space(&s);
    
    const char* name_start = s;
    while (*s && *s != ',') {
        s++;
    }
    if (*s != ',') {
        return false;
    }
    
    size_t name_len = s - name_start;
    for (size_t i = 0; i < name_len; i++) {
        name_buf[i] = name_start[i];
    }
    name_buf[name_len] = '\0';

    char* end = name_buf + name_len - 1;
    while (end >= name_buf && isspace(*end)) {
        *end = '\0';
        end--;
    }
    
    s++;
    skip_space(&s);
    
    char* endptr;
    long phone_val = strtol(s, &endptr, 10);
    if (endptr == s) {
        return false;
    }
    s = endptr;
    skip_space(&s);
    
    if (*s != ',') {
        return false;
    }
    s++;
    skip_space(&s);
    
    long group_val = strtol(s, &endptr, 10);
    if (endptr == s) {
        return false;
    }
    s = endptr;
    skip_space(&s);

    if (*s != ')') {
        return false;
    }
    s++;
    skip_space(&s);
    
    if (*s != ';') {
        return false;
    }
    
    type = command_type::insert;
    init(name_buf, static_cast<int>(phone_val), static_cast<int>(group_val));
    return true;
}

bool command::parse_select(const char* string) {
    type = command_type::select;
    
    const char* s = string;
    skip_space(&s);
    
    std::string output_fields;
    while (*s && *s != ';' && 
           strncmp(s, "where", 5) != 0 && 
           strncmp(s, "order by", 8) != 0) {
        output_fields += *s;
        s++;
    }
    
    if (!parse_output_fields(output_fields.c_str())) {
        return false;
    }
    
    skip_space(&s);
    
    if (strncmp(s, "where", 5) == 0) {
        s += 5;
        skip_space(&s);
        
        std::string where_cond;
        while (*s && *s != ';' && strncmp(s, "order by", 8) != 0) {
            where_cond += *s;
            s++;
        }
        
        if (!parse_search_condition(where_cond.c_str())) {
            return false;
        }
        
        skip_space(&s);
    }
    
    if (strncmp(s, "order by", 8) == 0) {
        s += 8;
        skip_space(&s);
        
        std::string order_fields;
        while (*s && *s != ';') {
            order_fields += *s;
            s++;
        }
        
        if (!parse_order_by(order_fields.c_str())) {
            return false;
        }
        
        skip_space(&s);
    }
    
    if (*s != ';') {
        return false;
    }
    
    return true;
}

bool command::parse_delete(const char* string) {
    type = command_type::del;
    
    const char* s = string;
    skip_space(&s);
    
    if (*s == ';') {
        return true;
    }
    
    if (strncmp(s, "where", 5) == 0) {
        s += 5;
        skip_space(&s);
        
        std::string where_cond;
        while (*s && *s != ';') {
            where_cond += *s;
            s++;
        }
        
        if (!parse_search_condition(where_cond.c_str())) {
            return false;
        }
        
        skip_space(&s);
    }
    
    if (*s != ';') {
        return false;
    }
    
    return true;
}

bool command::parse_output_fields(const char* fields) {
    while (isspace(*fields) || *fields == '\n') fields++;
    if (*fields == '\0') return false;
    
    std::unique_ptr<char[]> fields_copy(new char[strlen(fields) + 1]);
    strcpy(fields_copy.get(), fields);

    char* end = fields_copy.get() + strlen(fields_copy.get()) - 1;
    while (end > fields_copy.get() && (isspace(*end) || *end == '\n')) {
        *end = '\0';
        end--;
    }
    
    if (strcmp(fields_copy.get(), "*") == 0) {
        order[0] = ordering::name;
        order[1] = ordering::phone;
        order[2] = ordering::group;
        return true;
    }
    
    char* token = strtok(fields_copy.get(), ",");
    int index = 0;
    
    while (token && index < 3) {
        while (isspace(*token) || *token == '\n') token++;
        char* end = token + strlen(token) - 1;
        while (end > token && (isspace(*end) || *end == '\n')) {
            *end = '\0';
            end--;
        }
        
        ordering ord = ordering::none;
        if (strcmp(token, "name") == 0) ord = ordering::name;
        else if (strcmp(token, "phone") == 0) ord = ordering::phone;
        else if (strcmp(token, "group") == 0) ord = ordering::group;
        else return false;
        
        for (int i = 0; i < index; i++) {
            if (order[i] == ord) return false;
        }
        
        order[index++] = ord;
        token = strtok(NULL, ",");
    }
    
    return token == NULL;
}

bool command::parse_order_by(const char* fields) {
    while (isspace(*fields) || *fields == '\n') fields++;
    if (*fields == '\0') return false;
    
    std::unique_ptr<char[]> fields_copy(new char[strlen(fields) + 1]);
    strcpy(fields_copy.get(), fields);
    
    char* token = strtok(fields_copy.get(), ",");
    int index = 0;
    
    while (token && index < max_items) {
        while (isspace(*token) || *token == '\n') token++;
        char* end = token + strlen(token) - 1;
        while (end > token && (isspace(*end) || *end == '\n')) {
            *end = '\0';
            end--;
        }
        
        ordering ord = ordering::none;
        if (strcmp(token, "name") == 0) ord = ordering::name;
        else if (strcmp(token, "phone") == 0) ord = ordering::phone;
        else if (strcmp(token, "group") == 0) ord = ordering::group;
        else return false;
        
        for (int i = 0; i < index; i++) {
            if (order_by[i] == ord) return false;
        }
        
        order_by[index++] = ord;
        token = strtok(NULL, ",");
    }
    
    return token == NULL;
}

bool command::parse_search_condition(const char* condition) {
    if (!condition || *condition == '\0') return false;
    
    const char* and_pos = strstr(condition, " and ");
    const char* or_pos = strstr(condition, " or ");

    if (and_pos && (!or_pos || and_pos < or_pos)) {
        op = operation::land;
        
        std::string cond1(condition, and_pos - condition);
        std::string cond2(and_pos + 5);
        return parse_single_condition(cond1.c_str()) && 
               parse_search_condition(cond2.c_str());
    }
    else if (or_pos && (!and_pos || or_pos < and_pos)) {
        op = operation::lor;
        
        std::string cond1(condition, or_pos - condition);
        std::string cond2(or_pos + 4);
        
        return parse_single_condition(cond1.c_str()) &&
               parse_search_condition(cond2.c_str());
    }
    else {
        return parse_single_condition(condition);
    }
}

bool command::parse_single_condition(const char* string) {
    while (isspace(*string) || *string == '\n') string++;
    if (*string == '\0') return false;
    
    std::unique_ptr<char[]> str_copy(new char[strlen(string) + 1]);
    strcpy(str_copy.get(), string);
    
    char* field = strtok(str_copy.get(), " \t");
    if (!field) return false;
    
    char* op_str = strtok(NULL, " \t");
    if (!op_str) return false;
    
    condition cond;
    if (strcmp(op_str, "not") == 0) {
        char* like_op = strtok(NULL, " \t");
        if (!like_op || strcmp(like_op, "like") != 0) {
            return false;
        }
        cond = condition::nlike;
    } else {
        cond = str_to_condition(op_str);
        if (cond == condition::none) {
            return false;
        }
    }
    
    char* value = strtok(NULL, "");
    if (!value) return false;
    
    while (isspace(*value) || *value == '\n') value++;
    if (*value == '\0') return false;
    
    char* end = value + strlen(value) - 1;
    while (end > value && (isspace(*end) || *end == '\n')) {
        *end = '\0';
        end--;
    }
    
    if (strcmp(field, "name") == 0) {
        if (c_name != condition::none) return false;
        c_name = cond;
        init(value, get_phone(), get_group());
        return true;
    }
    else if (strcmp(field, "phone") == 0) {
        if (c_phone != condition::none) return false;
        if (cond == condition::like || cond == condition::nlike) return false;
        
        c_phone = cond;
        char* endptr;
        long val = strtol(value, &endptr, 10);
        if (*endptr != '\0') return false;
        
        init(get_name(), static_cast<int>(val), get_group());
        return true;
    }
    else if (strcmp(field, "group") == 0) {
        if (c_group != condition::none) return false;
        if (cond == condition::like || cond == condition::nlike) return false;
        
        c_group = cond;
        char* endptr;
        long val = strtol(value, &endptr, 10);
        if (*endptr != '\0') return false;
        
        init(get_name(), get_phone(), static_cast<int>(val));
        return true;
    }
    
    return false;
}

bool command::apply(const record& x) const {
    bool def_val = (op == operation::land);
    
    bool name_res = (c_name == condition::none) ? def_val : 
                    x.compare_name(c_name, *this);
    bool phone_res = (c_phone == condition::none) ? def_val : 
                     x.compare_phone(c_phone, *this);
    bool group_res = (c_group == condition::none) ? def_val : 
                     x.compare_group(c_group, *this);

    if (op == operation::land) return name_res && phone_res && group_res;
    if (op == operation::lor) return name_res || phone_res || group_res;
    
    if (c_name != condition::none) return name_res;
    if (c_phone != condition::none) return phone_res;
    if (c_group != condition::none) return group_res;
    return true;
}

int command::compare_records(const record& a, const record& b, const ordering order_by[]) {
    for (int i = 0; i < 3; i++) {
        if (order_by[i] == ordering::none) break;
        
        int cmp = 0;
        switch (order_by[i]) {
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
            default:
                break;
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
        case command_type::del: fprintf(fp, "DELETE\n"); break;
        default: fprintf(fp, "NONE\n"); return;
    }
    
    if (type == command_type::select) {
        fprintf(fp, "  Output fields: ");
        bool first = true;
        for (int i = 0; i < 3; i++) {
            if (order[i] != ordering::none) {
                if (!first) fprintf(fp, ", ");
                switch (order[i]) {
                    case ordering::name: fprintf(fp, "name"); break;
                    case ordering::phone: fprintf(fp, "phone"); break;
                    case ordering::group: fprintf(fp, "group"); break;
                    default: break;
                }
                first = false;
            }
        }
        fprintf(fp, "\n");
    }
    
    if (c_name != condition::none || c_phone != condition::none || c_group != condition::none) {
        fprintf(fp, "  Conditions: ");
        if (c_name != condition::none) fprintf(fp, "name ");
        if (c_phone != condition::none) fprintf(fp, "phone ");
        if (c_group != condition::none) fprintf(fp, "group ");
        if (op == operation::land) fprintf(fp, "(AND)");
        if (op == operation::lor) fprintf(fp, "(OR)");
        fprintf(fp, "\n");
    }
    
    if (type == command_type::select) {
        bool first = true;
        for (int i = 0; i < 3; i++) {
            if (order_by[i] != ordering::none) {
                if (first) {
                    fprintf(fp, "  Order by: ");
                    first = false;
                } else {
                    fprintf(fp, ", ");
                }
                switch (order_by[i]) {
                    case ordering::name: fprintf(fp, "name"); break;
                    case ordering::phone: fprintf(fp, "phone"); break;
                    case ordering::group: fprintf(fp, "group"); break;
                    default: break;
                }
            }
        }
        if (!first) fprintf(fp, "\n");
    }
}