#include <iostream>
#include <queue>
#include <vector>

#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/time.h>

#include "m_buffer.h"

// #define DSLOG_TAG "M_BUFFER"
// #include "../includes/ds_log.h"

struct sem_thread{
    sem_t *semaphore;
    pthread_t pthread_id;
    pthread_mutex_t *p_mutex_p;
    pthread_mutex_t *c_mutex_p;
    std::queue<struct buffer_task *> *p_queue_p;
    std::queue<struct buffer_task *> *c_queue_p;
};

struct timetick_thread{
    pthread_t pthread_id;
    uint32_t p_thread_size;
    std::vector<struct sem_thread> *p_threads_p;
    sem_thread *c_thread_p;
};


M_buffer::M_buffer(struct buffer_task **bts_p, uint32_t bts_num){
    uint32_t i = 0;
    this->buffer_task_size = bts_num;
    if (bts_p == NULL){
        return;
    }
    struct buffer_task *bts = *bts_p;
    this->consume_thread = (struct sem_thread *)malloc(sizeof(struct sem_thread));
    for (i=0; i < MAX_PRODUCE_THREAD_NUM; i++){
        this->sems[i] = (sem_t *)malloc(sizeof(sem_t));
    }
    this->consume_sem = (sem_t *)malloc(sizeof(sem_t));
    this->timetick_thread = (struct timetick_thread *)malloc(sizeof(struct timetick_thread));
    this->p_queue = new std::queue<struct buffer_task *>;
    this->c_queue = new std::queue<struct buffer_task *>;
    this->produce_threads = new std::vector<struct sem_thread>;
    this->p_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    this->c_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    
    pthread_mutex_init(this->p_mutex,NULL);
    pthread_mutex_init(this->c_mutex,NULL);
    pthread_mutex_lock(this->p_mutex);
    for (i = 0; i < bts_num; i++){
        p_queue->push(&(bts[i]));
    }
    pthread_mutex_unlock(this->p_mutex);

    this->produce_threads->resize(MAX_PRODUCE_THREAD_NUM);
    for (i = 0; i < this->produce_threads->size(); i++){
        sem_init(this->sems[i],0,0);
        (*this->produce_threads)[i].semaphore = this->sems[i];
        (*this->produce_threads)[i].p_queue_p = this->p_queue;
        (*this->produce_threads)[i].c_queue_p = this->c_queue;
        (*this->produce_threads)[i].p_mutex_p = this->p_mutex;
        (*this->produce_threads)[i].c_mutex_p = this->c_mutex;
    }

    sem_init(this->consume_sem,0,0);
    this->consume_thread->semaphore = this->consume_sem;
    this->consume_thread->p_queue_p = this->p_queue;
    this->consume_thread->c_queue_p = this->c_queue;
    this->consume_thread->p_mutex_p = this->p_mutex;
    this->consume_thread->c_mutex_p = this->c_mutex;

    this->timetick_thread->p_thread_size = MAX_PRODUCE_THREAD_NUM;
    this->timetick_thread->c_thread_p = this->consume_thread;
    this->timetick_thread->p_threads_p = this->produce_threads;
}

M_buffer::~M_buffer(){
    uint32_t i;
    free(this->p_mutex);
    free(this->c_mutex);
    delete this->produce_threads;
    delete this->p_queue;
    delete this->c_queue;
    free(this->timetick_thread);
    free (this->consume_sem);
    free(this->consume_thread);
    for ( i = 0; i < this->buffer_task_size; i++){
        free(this->sems[i]);
    }
}


void *produce_thread_handle(void *arg){
    struct sem_thread *p_thread_p =  (struct sem_thread *)arg;
    for(;;){
        sem_wait(p_thread_p->semaphore);
        struct buffer_task *p_task = NULL;
        if (!pthread_mutex_trylock(p_thread_p->p_mutex_p)){
            if (!(p_thread_p->p_queue_p->empty())){
                p_task = p_thread_p->p_queue_p->front();
                p_thread_p->p_queue_p->pop();
            }
            pthread_mutex_unlock(p_thread_p->p_mutex_p);
        }
        if (p_task != NULL && p_task->produce_task != NULL){
            p_task->produce_task(p_task->args);
            // TODO 这里按理说要对task进行排序
            pthread_mutex_lock(p_thread_p->c_mutex_p);
            p_thread_p->c_queue_p->push(p_task);
            pthread_mutex_unlock(p_thread_p->c_mutex_p);
        }
    }
}

void *consume_thread_handle(void *arg){
    struct sem_thread *c_thread_p = (struct sem_thread *)arg;
    for(;;){
        sem_wait(c_thread_p->semaphore);
        struct buffer_task *c_task = NULL;
        pthread_mutex_lock(c_thread_p->c_mutex_p);
        if (!(c_thread_p->c_queue_p->empty())){
            c_task = c_thread_p->c_queue_p->front();
            c_thread_p->c_queue_p->pop();
        }
        pthread_mutex_unlock(c_thread_p->c_mutex_p);
        if (c_task != NULL && c_task->consume_task != NULL){
            c_task->consume_task(c_task->args);
            pthread_mutex_lock(c_thread_p->p_mutex_p);
            c_thread_p->p_queue_p->push(c_task);
            pthread_mutex_unlock(c_thread_p->p_mutex_p);
        }
    }
}

void *timetick_thread_handle(void *arg){
    struct timetick_thread *tt_thread_p = (struct timetick_thread *)arg;
    uint32_t p_thread_size = tt_thread_p->p_thread_size;
    std::vector<struct sem_thread> p_threads_p = *(tt_thread_p->p_threads_p);
    sem_thread *c_thread_p = tt_thread_p->c_thread_p;
    uint32_t i = 0;
    for(;;){
        for(i = 0; i < p_thread_size; i++){
            sem_post(p_threads_p[i].semaphore);
        }
        sem_post(c_thread_p->semaphore);
        usleep(15000);
    }
}


uint8_t M_buffer::start_m_buffer_thread(){
    uint32_t i = 0;
    if (p_queue->empty()){
        printf("No buffer task\n");
        return -1;
    }

    printf("Start multi-buffer model\n");

    if (pthread_create(&(this->consume_thread->pthread_id),NULL,consume_thread_handle,(void *)(this->consume_thread)) != 0){
        perror("flush framebuffer thread");
        return -1;
    }

    for (i = 0; i < MAX_PRODUCE_THREAD_NUM; i++){
        if(pthread_create(&((*this->produce_threads)[i].pthread_id), NULL, produce_thread_handle, (void *)(&((*this->produce_threads)[i])))){
            perror("produce framebuffer thread");
            continue;
        }
    }

    if (pthread_create(&(this->timetick_thread->pthread_id),NULL,timetick_thread_handle, (void *)(this->timetick_thread)) != 0){
        perror("time tick thread");
        return -1;
    }
}

// static uint32_t g_serial = 0;

// void *produce_task_func(void *arg){
//     g_serial++;
//     int *value = (int *)arg;
//     *value = g_serial;
//     usleep(50000);
// }


// static struct timeval curr_time, last_time;
// void *consume_task_func(void *arg){
//     int *value = (int *)arg;
    
    
//     double time_stamp,last_time_stamp,time_splite;
//     last_time_stamp = (curr_time.tv_sec*1000000.0+curr_time.tv_usec)/1000;
//     gettimeofday(&curr_time,NULL);
//     time_stamp = (curr_time.tv_sec*1000000.0+curr_time.tv_usec)/1000;
//     time_splite = time_stamp - last_time_stamp;
    
//     printf("[%lf][%lf]<<<<<<<<<< show value = %d, pthread = %ld\n",time_stamp,time_splite,*value,pthread_self());
//     usleep(10000);
// }


// int main(int argc, char const *argv[])
// {
//     uint32_t i;
//     uint32_t ret;
//     struct buffer_task *buffer_task;
//     uint32_t task_values[BUFFER_TASK_SIZE];
    
//     buffer_task = (struct buffer_task *)malloc(BUFFER_TASK_SIZE * sizeof(struct buffer_task));
//     for (i = 0; i < BUFFER_TASK_SIZE; i++){
//         buffer_task[i].args =  (void *)&(task_values[i]);
//         buffer_task[i].produce_task = produce_task_func;
//         buffer_task[i].consume_task = consume_task_func;
//     }

//     M_buffer m_buffer(&buffer_task,BUFFER_TASK_SIZE);
//     m_buffer.start_m_buffer_thread();

//     for(;;){
//         usleep(10000);
//     }
//     free(buffer_task);
//     return 0;
// }
