#ifndef __M_BUFFER_H__
#define __M_BUFFER_H__

#include <queue>
#include <vector>
#include <pthread.h>
#include <semaphore.h>

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

class M_buffer
{
    private:
        uint32_t buffer_task_size = 0;
        std::vector<struct sem_thread> *produce_threads;
        sem_t *sems[MAX_PRODUCE_THREAD_NUM], *consume_sem;
        struct sem_thread *consume_thread;
        struct timetick_thread *timetick_thread;
        std::queue<struct buffer_task *> *p_queue;
        std::queue<struct buffer_task *> *c_queue;
        pthread_mutex_t *p_mutex,*c_mutex;
    public:
        M_buffer(struct buffer_task **bts, uint32_t bts_num);
        ~M_buffer();

        uint8_t start_m_buffer_thread();
};

#endif