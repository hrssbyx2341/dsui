#ifndef __FLASH_FB_H__
#define __FLASH_FB_H__

#include <semaphore.h>
#include <stdint.h>
#include <pthread.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef uint64_t fb_serial_t;
// typedef uint8_t fb_serial_t;

#define FRAMEBUFFER_NUMBER 3

struct framebuffer{
    uint32_t height;
    uint32_t weight;
    uint32_t bpp;
	uint32_t size;
	uint32_t handle;	
	uint32_t fb_id;
	uint32_t *vaddr;	
};

struct fb_box{
    uint8_t box_id;
	fb_serial_t fb_serial;
    uint8_t is_displayed;
    struct framebuffer fb;
    pthread_mutex_t mutex_lock;
    sem_t semaphore;
};

struct fb_global_info{
    fb_serial_t g_fb_serial;
    fb_serial_t g_ready_fb_serial;
    pthread_mutex_t ready_fb_mutex;
};

int start_flash_buffer();

#endif