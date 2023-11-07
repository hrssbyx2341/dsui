#ifndef __FLASH_BUFFER_H__
#define __FLASH_BUFFER_H__

#include <semaphore.h>
#include <stdint.h>
#include <pthread.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

// typedef uint64_t fb_serial_t;
typedef uint8_t fb_serial_t;

#define FRAMEBUFFER_NUMBER 3


struct framebuffer{
    struct framebuffer *next, *prev;
    uint32_t fb_id;
	uint32_t number;
    pthread_mutex_t mutex_lock;
    sem_t semaphore;
};

int start_flash_buffer();

#endif