#ifndef __INPUT_READER_H__
#define __INPUT_READER_H__

#define MAX_INPUT_DEVICE 256

#include <linux/input.h>
#include <stdint.h>
#include <sys/epoll.h>

#define TRUE 1
#define FALSE 0

#define EVENT_ADD 1
#define EVENT_REMOVE 2

typedef int8_t (*input_callback_func)(struct input_event, int8_t);
typedef int8_t (*event_callback_func)(char*, int8_t);


struct callback_func{
    input_callback_func input_callback;
    event_callback_func event_callback;
};

struct hardware_input
{
    int8_t is_avaiable;
    char *device_name;
    struct epoll_event ev;
    struct input_event event;
};

int8_t init_input(struct callback_func *);



#endif