#ifndef IO_STATUS_H
#define IO_STATUS_H

enum class io_status {
    success,
    eof,
    format,
    memory,
    open,
    create
};

#endif
