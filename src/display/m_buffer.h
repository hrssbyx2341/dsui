#ifndef __M_BUFFER_H__
#define __M_BUFFER_H__

#ifndef MAX_PRODUCE_THREAD_NUM
#define MAX_PRODUCE_THREAD_NUM 10
#endif

#ifndef BUFFER_TASK_SIZE
#define BUFFER_TASK_SIZE 20
#endif

typedef void* (*task_func_t)(void *);

struct buffer_task{
    void *args;
    task_func_t produce_task;
    task_func_t consume_task;
};

#endif