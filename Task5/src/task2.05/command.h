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
    ordering order[max_items] = {ordering::none, ordering::none, ordering::none};
    ordering order_by[max_items] = {ordering::none, ordering::none, ordering::none};

public:
    command() = default;
    ~command() = default;

    bool parse(const char *string);
    void print(FILE *fp = stdout) const;
    bool apply(const record& x) const;
    
    command_type get_type() const { return type; }
    const ordering* get_order() const { return order; }
    const ordering* get_order_by() const { return order_by; }
    
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
};

#endif // COMMAND_H