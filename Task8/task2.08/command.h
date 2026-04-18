#ifndef COMMAND_H
#define COMMAND_H
#include <stdio.h>
#include "record.h"
#include "operation.h"
#include "ordering.h"
#include "command_type.h"

class command : public record
{
private:
    static const int max_items = 3;
    command_type type = command_type::none;
    condition c_name = condition::none;
    condition c_phone = condition::none;
    condition c_group = condition::none;
    operation op = operation::none;
    ordering order[max_items] = { };
    ordering order_by[max_items] = { };

public:
    command() = default;
    ~command() = default;

    bool parse(const char *string);
    void print(FILE *fp = stdout) const;
    bool apply(const record& x) const;
    
    command_type get_type() const { return type; }
    const ordering* get_order() const { return order; }
    const ordering* get_order_by() const { return order_by; }

    bool has_name_eq() const { return c_name == condition::eq; }
    bool has_name_condition() const { return c_name != condition::none; }
    const char* get_name_value() const { return get_name(); }
    condition get_name_cond() const { return c_name; }

    bool has_phone_eq() const { return c_phone == condition::eq; }
    bool has_phone_condition() const { return c_phone != condition::none; }
    condition get_phone_cond() const { return c_phone; }
    int get_phone_value() const { return get_phone(); }

    bool has_group_eq() const { return c_group == condition::eq; }
    bool has_group_condition() const { return c_group != condition::none; }
    int get_group_value() const { return get_group(); }

    bool has_op_land() const { return op == operation::land; }
    bool has_op_lor() const { return op == operation::lor; }
    bool has_op_none() const { return op == operation::none; }
    
    static int compare_records(const record& a, const record& b, const ordering order_by[]);

private:
    bool parse_output_fields(const char* fields);
    bool parse_search_condition(const char* condition);
    bool parse_single_condition(const char* string);
    bool parse_order_by(const char* fields);

    bool parse_quit(const char* string);
    bool parse_insert(const char* string);
    bool parse_select(const char* string);
    bool parse_delete(const char* string);
    
    static void skip_space(const char** str);
    void reset();
};

#endif
