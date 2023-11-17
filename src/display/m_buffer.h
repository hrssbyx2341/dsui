#ifndef __M_BUFFER_H__
#define __M_BUFFER_H__

#define MAX_PRODUCE_THREAD_NUM 3

typedef void* (*task_func_t)(void *);

struct buffer_task{
    void *args;
    task_func_t produce_task;
    task_func_t consume_task;
};

#endif