#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <linux/mutex.h>

#define BUFFER_SIZE 1024

struct shared_data {
    char buffer[BUFFER_SIZE];
    int read_pos;
    int write_pos;
    struct mutex buffer_mutex;
};

extern struct shared_data shared_memory;

#endif

