#ifndef __HARDWARE_INPUTS_H__
#define __HARDWARE_INPUTS_H__

#define MAX_INPUT_DEVICE 30

#include <linux/input.h>
#include <stdint.h>

#define TRUE 1
#define FALSE 0

#define SET_INUT_CALLBACK(func) input_callback=func

int8_t *(input_callback)(struct input_event, int8_t device_id);

// int8_t init_input(void);

struct hardware_inputs
{
    int8_t is_avaiable;
    char *device_name;
    int32_t device_id;
    struct input_event;
};


#endif