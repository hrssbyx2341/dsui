#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#include "flash_buffer.h"

#define DSLOG_TAG "FLASH_BUFFER"
#include "../../includes/ds_log.h"


static struct framebuffer g_fbs[FRAMEBUFFER_NUMBER];
static struct framebuffer *g_front_fb;
static struct framebuffer *g_back_fb;
static sem_t flash_semaphore;

uint32_t g_num = 0;

uint32_t init_framebuffers(){
    uint32_t i = 0;
    // TODO 这里对 g_fbs[0] 进行初始化
    g_fbs[0].fb_id = 0;
    g_front_fb = &(g_fbs[0]);
    for (i = 1; i < FRAMEBUFFER_NUMBER; i++){
        // TODO 这里对 g_fbs[i] 进行初始化
        g_fbs[i].fb_id = i;
        g_front_fb->next = &(g_fbs[i]);
        g_front_fb = g_front_fb->next;
    }
    g_front_fb->next = &(g_fbs[0]);
    g_back_fb = g_front_fb;
    return 0;
}


void show_buffer(struct framebuffer *fb){
    DSLOGD("flash frame %d, fb id = %d\n",fb->number, fb->fb_id);
    usleep(20000);
}

void *flash_buffer(void *arg){
    for(;;){
        sem_wait(&flash_semaphore);
        DSLOGD("xiaohu --->\n");
        struct framebuffer *front_fb = g_front_fb;
        if (!pthread_mutex_trylock(&(front_fb->mutex_lock))){ // 这里展示帧已经准备好了
            // TODO 这里刷图
            show_buffer(front_fb);
            pthread_mutex_unlock(&(front_fb->mutex_lock));
        }else{
            DSLOGD("%d frame buffer is reading\n",front_fb->fb_id);
        }
    }
    
}

void draw_buffer(struct framebuffer *fb){
    DSLOGD("fill frame buffer by %ld, and fill num = %d, fb_id = %u\n",pthread_self(), g_num, fb->fb_id);
    fb->number = g_num;
    g_num++;
    usleep(50000);
}

void *fill_buffer(void *arg){
    struct framebuffer *back_buffer = (struct framebuffer *)arg;
    for(;;){
        sem_wait(&(back_buffer->semaphore));
        if (!pthread_mutex_lock(&(back_buffer->mutex_lock))){ //这里说明这一帧未被显示，可以作为准备帧
            // TODO 这里填充 framebuffer
            draw_buffer(back_buffer);
            pthread_mutex_unlock(&(back_buffer->mutex_lock));
        }else{
            DSLOGD("%d frame buffer is showing\n",back_buffer->fb_id);
        }
    }
}

void *time_tick(void *arg){
    for(;;){
        usleep(16000);
        sem_post(&(g_back_fb->semaphore));
        g_back_fb = g_back_fb->next;
        sem_post(&flash_semaphore);
        g_front_fb = g_front_fb->next;
    }
}

int start_flash_buffer(){
    pthread_t flash_buffer_pthread, fill_buffer_pthread[FRAMEBUFFER_NUMBER], time_tick_pthread;
    uint32_t i = 0, ret = 0;

    ret = init_framebuffers();
    if (ret != 0){
        DSLOGE("init framebuffer failed\n");
        return -1;
    }
    DSLOGD("Start flash buffer\n");
    
    if (pthread_create(&flash_buffer_pthread,NULL,flash_buffer,NULL) != 0){
        perror("flush framebuffer thread");
        return -1;
    }

    for (i = 0; i < FRAMEBUFFER_NUMBER; i++){
        if(pthread_create(&fill_buffer_pthread[i], NULL, fill_buffer, (void *)(&(g_fbs[i])))){
            perror("fill framebuffer thread");
            continue;
        }
    }

    if (pthread_create(&time_tick_pthread,NULL,time_tick, NULL) != 0){
        perror("time tick thread");
        return -1;
    }

    pthread_join(flash_buffer_pthread, NULL);
    for (i = 0; i < FRAMEBUFFER_NUMBER; i++){
        pthread_join(fill_buffer_pthread[i],NULL);
    }
    pthread_join(time_tick_pthread, NULL);

    return 0;
}

int main(int argc, char const *argv[])
{
    start_flash_buffer();
    return 0;
}
