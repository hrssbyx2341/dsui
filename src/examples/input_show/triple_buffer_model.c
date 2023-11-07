#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define BUFFER_SIZE 10

int buffer[BUFFER_SIZE];
int count = 0;  // 当前队列中的元素个数
int prod_count = 0;  // 生产者生成的物品计数

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  // 互斥锁
pthread_cond_t cond_producer = PTHREAD_COND_INITIALIZER;  // 生产者条件变量
pthread_cond_t cond_consumer = PTHREAD_COND_INITIALIZER;  // 消费者条件变量

void* producer(void* arg) {
    int item;

    while (1) {
        item = rand() % 100;  // 生成随机数作为生产的物品

        pthread_mutex_lock(&mutex);  // 加锁

        while (count == BUFFER_SIZE) {
            // 队列已满，等待消费者消费物品后再继续生产
            pthread_cond_wait(&cond_producer, &mutex);
        }

        buffer[count] = item;  // 将物品放入队列
        count++;
        prod_count++;

        printf("生产者 %ld 生产物品: %d\n", (long)arg, item);

        pthread_cond_signal(&cond_consumer);  // 唤醒消费者线程
        pthread_mutex_unlock(&mutex);  // 解锁
    }

    return NULL;
}

void* consumer(void* arg) {
    int item;
    int cons_count = 0;  // 消费者消费的物品计数

    while (1) {
        pthread_mutex_lock(&mutex);  // 加锁

        while (count == 0 || cons_count >= prod_count) {
            // 队列为空或者消费者已经消费了所有生产者生成的物品，等待生产者继续生产
            pthread_cond_wait(&cond_consumer, &mutex);
        }

        item = buffer[cons_count % BUFFER_SIZE];  // 从队列中取出物品
        cons_count++;

        printf("消费者消费物品: %d\n", item);

        pthread_cond_signal(&cond_producer);  // 唤醒生产者线程
        pthread_mutex_unlock(&mutex);  // 解锁
    }

    return NULL;
}

int main() {
    pthread_t producer1, producer2, producer3, consumer1;

    // 创建生产者线程
    pthread_create(&producer1, NULL, producer, (void*)1);
    pthread_create(&producer2, NULL, producer, (void*)2);
    pthread_create(&producer3, NULL, producer, (void*)3);

    // 创建消费者线程
    pthread_create(&consumer1, NULL, consumer, NULL);

    // 等待线程结束
    pthread_join(producer1, NULL);
    pthread_join(producer2, NULL);
    pthread_join(producer3, NULL);
    pthread_join(consumer1, NULL);

    return 0;
}
