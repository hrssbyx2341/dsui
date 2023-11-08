#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

#include "flash_fb.h"

#define DSLOG_TAG "FLASH_BUFFER"
#include "../includes/ds_log.h"


static struct fb_box g_fbs[FRAMEBUFFER_NUMBER];
static sem_t flash_semaphore;
static struct fb_global_info g_fbi;

uint32_t g_num = 0;

struct timeval last_time, curr_time;

uint32_t init_global_fb_info(){
    g_fbi.g_fb_serial = 0;
    g_fbi.g_ready_fb_serial = 0;
}
uint32_t init_fb_boxes(){
    uint32_t i = 0;
    for (i = 0; i < FRAMEBUFFER_NUMBER; i++){
        g_fbs[i].box_id = i;
        g_fbs[i].is_displayed = TRUE;
        g_fbs[i].fb_serial = 0;
    }
    return 0;
}


void show_buffer(struct fb_box *fb){
    // uint32_t fps;
    // double time_stamp, last_time_stamp;
    // last_time.tv_sec = curr_time.tv_sec;
    // last_time.tv_usec = curr_time.tv_usec;
    // gettimeofday(&curr_time,NULL);
    // last_time_stamp = (last_time.tv_sec*1000000.0+last_time.tv_usec)/1000;
    // time_stamp = (curr_time.tv_sec*1000000.0+curr_time.tv_usec)/1000;
    // fps = 1000/(time_stamp - last_time_stamp);

    // DSLOGD("[FPS=%d][%lf]flash frame %ld, fb id = %d, ready fb =%ld\n",fps,time_stamp
    //     ,fb->fb_serial, fb->box_id, g_fbi.g_ready_fb_serial);
    fb->is_displayed = TRUE;
    usleep(5000);
}

void *flash_buffer(void *arg){
    uint8_t i = 0;
    for(;;){
        sem_wait(&flash_semaphore);
        for( i = 0 ; i < FRAMEBUFFER_NUMBER; i++){
            if((g_fbs[i].fb_serial == 0) || (g_fbs[i].fb_serial-1 == (g_fbi.g_fb_serial))){
                if (!(g_fbs[i].is_displayed)){
                    show_buffer(&(g_fbs[i]));
                    g_fbi.g_fb_serial++;
                }
            }
        }
    }
    
}

void draw_buffer(struct fb_box *fb){
    // DSLOGD("fill frame, fb id = %d, ready fb = %ld\n",fb->box_id,g_fbi.g_ready_fb_serial);
    fb->is_displayed=FALSE;
    usleep(50000);
}

void *fill_buffer(void *arg){
    struct fb_box *ready_fb = (struct fb_box *)arg;
    for(;;){
        sem_wait(&(ready_fb->semaphore));
        if (ready_fb->is_displayed){
            // TODO 这里填充 framebuffer
            g_fbi.g_ready_fb_serial++;
            ready_fb->fb_serial = g_fbi.g_ready_fb_serial;
            draw_buffer(ready_fb);
        }else{
            DSLOGD("%d frame buffer is showing\n",ready_fb->box_id);
        }
    }
}

void *time_tick(void *arg){
    uint8_t i = 0;
    for(;;){
        usleep(16000);
        for ( i = 0; i < FRAMEBUFFER_NUMBER; i++){
            if (g_fbs[i].is_displayed){
                sem_post(&(g_fbs[i].semaphore));
                break;
            }
        }
        sem_post(&flash_semaphore);
    }
}

int start_flash_buffer(){
    pthread_t flash_buffer_pthread, fill_buffer_pthread[FRAMEBUFFER_NUMBER], time_tick_pthread;
    uint32_t i = 0, ret = 0;

    init_global_fb_info();
    ret = init_fb_boxes();
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
