#ifndef command_type_H
#define command_type_H

enum class command_type {
    none,     // uninitialized
    quit,     // end of client session
    insert,   // add record
    select,   // find by conditions specified
    del,      // delete record
    stop      // terminate server
};

#endif // command_type_H
