#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

#define BUFFER_SIZE 10

int buffer[BUFFER_SIZE];
int count = 0;
int in = 0;
int out = 0;

sem_t empty_slots;
sem_t filled_slots;
pthread_mutex_t mutex;

void* producer(void* arg) {
    int item = 1;
    while (1) {
        // 等待空槽位
        sem_wait(&empty_slots);
        
        // 获取互斥锁
        pthread_mutex_lock(&mutex);
        
        // 生产物品
        buffer[in] = item;
        printf("Produced item: %d\n", item);
        item++;
        in = (in + 1) % BUFFER_SIZE;
        count++;
        
        // 释放互斥锁
        pthread_mutex_unlock(&mutex);
        
        // 通知有物品可供消费
        sem_post(&filled_slots);
    }
}

void* consumer(void* arg) {
    while (1) {
        // 等待有物品可供消费
        sem_wait(&filled_slots);
        
        // 获取互斥锁
        pthread_mutex_lock(&mutex);
        
        // 消费物品
        int item = buffer[out];
        printf("Consumed item: %d\n", item);
        out = (out + 1) % BUFFER_SIZE;
        count--;
        
        // 释放互斥锁
        pthread_mutex_unlock(&mutex);
        
        // 通知有空槽位可供生产
        sem_post(&empty_slots);
    }
}

int main() {
    pthread_t producer1, producer2, producer3, consumer1;
    
    // 初始化信号量和互斥锁
    sem_init(&empty_slots, 0, BUFFER_SIZE);
    sem_init(&filled_slots, 0, 0);
    pthread_mutex_init(&mutex, NULL);
    
    // 创建生产者线程
    pthread_create(&producer1, NULL, producer, NULL);
    pthread_create(&producer2, NULL, producer, NULL);
    pthread_create(&producer3, NULL, producer, NULL);
    
    // 创建消费者线程
    pthread_create(&consumer1, NULL, consumer, NULL);
    
    // 等待线程结束
    pthread_join(producer1, NULL);
    pthread_join(producer2, NULL);
    pthread_join(producer3, NULL);
    pthread_join(consumer1, NULL);
    
    // 销毁信号量和互斥锁
    sem_destroy(&empty_slots);
    sem_destroy(&filled_slots);
    pthread_mutex_destroy(&mutex);
    
    return 0;
}
