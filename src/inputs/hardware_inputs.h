#ifndef __HARDWARE_INPUTS_H__
#define __HARDWARE_INPUTS_H__

#define MAX_INPUT_DEVICE 30

#include <linux/input.h>
#include <stdint.h>
#include <sys/epoll.h>

#define TRUE 1
#define FALSE 0

typedef int8_t *(input_callback_func)(struct input_event, int8_t);

int8_t init_input(input_callback_func);

struct hardware_input
{
    int8_t is_avaiable;
    char *device_name;
    struct epoll_event ev;
    struct input_event event;
};




#endif