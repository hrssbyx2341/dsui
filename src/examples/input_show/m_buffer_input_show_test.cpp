#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/input-event-codes.h>
#include <sys/time.h>
#include <cmath>
#include <chrono>
#include <thread>



#include "xf86drm.h"
#include "xf86drmMode.h"
#include "../../inputs/input_reader.h"

// #define DSLOG_TAG "INPUT_SHOW"
// #include "../../includes/ds_log.h"

#define MAX_PRODUCE_THREAD_NUM 1
#define BUFFER_TASK_SIZE 3
#include "../../display/m_buffer.h"



#define MIW 20
struct mouse_pos{
   uint32_t x;
   uint32_t y;
   uint32_t max_x;
   uint32_t max_y;
   uint32_t min_x;
   uint32_t min_y;
};

static struct mouse_pos g_mp, g_last_mp;

uint32_t update_last_mp(){
    uint32_t ret = 0;
    if(g_last_mp.min_x != g_mp.min_x){
        g_last_mp.min_x = g_mp.min_x;
        ret = 1;
    }
    if (g_last_mp.max_x != g_mp.max_x){
        g_last_mp.max_x = g_mp.max_x;
        ret = 1;
    }
    if (g_last_mp.min_y != g_mp.min_y){
        g_last_mp.min_y = g_mp.min_y;
        ret = 1;
    }
    if (g_last_mp.max_y != g_mp.max_y){
        g_last_mp.max_y = g_mp.max_y;
        ret = 1;
    }
    if (g_last_mp.x != g_mp.x){
        g_last_mp.x = g_mp.x;
        ret = 1;
    }
    if (g_last_mp.y != g_mp.y){
        g_last_mp.y = g_mp.y;
        ret = 1;
    }
    return ret;
}

void init_mp(int32_t min_x, int32_t min_y, int32_t max_x, int32_t max_y){
    g_mp.min_x = min_x;
    g_mp.min_y = min_y;
    g_mp.max_x = max_x;
    g_mp.max_y = max_y;
    g_mp.x = (max_x+min_x)/2;
    g_mp.y = (max_y+min_y)/2;
    update_last_mp();
    //TODO 这里开始显示鼠标点
}

int8_t test_input_callback(struct input_event event, int8_t device_id){
    // printf("Event id = %d, event type = 0x%x, code = 0x%x, value = 0x%x\n",device_id,event.type,event.code,event.value);
    switch (event.type)
    {
    case EV_SYN:
            // printf("mouse pos = %dx%d\n",g_mp.x,g_mp.y);
            // show_mouse();
        break;
    case EV_REL:
        switch (event.code)
        {
        case REL_X:
            // printf("mouse move x = %d\n",event.value);
            g_mp.x += event.value;
            if(g_mp.x <= g_mp.min_x) g_mp.x = g_mp.min_x;
            if(g_mp.x >= g_mp.max_x) g_mp.x = g_mp.max_x;
            break;
        case REL_Y:
            // printf("mouse move y = %d\n",event.value);
            g_mp.y += event.value;
            if(g_mp.y <= g_mp.min_y) g_mp.y = g_mp.min_y;
            if(g_mp.y >= g_mp.max_y) g_mp.y = g_mp.max_y;
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}

int8_t test_event_callback(char *event_name, int8_t change_type){
    printf("%s is %s\n",event_name,(change_type==EVENT_ADD?"added":"removed"));
}

#define uint32_t unsigned int 
 
struct framebuffer{
    uint32_t width;
    uint32_t height;
	uint32_t size;
	uint32_t handle;	
	uint32_t fb_id;
	uint32_t *vaddr;	
};
 
static void release_fb(int fd, struct framebuffer *buf)
{
	struct drm_mode_destroy_dumb destroy = {};
	destroy.handle = buf->handle;
 
	drmModeRmFB(fd, buf->fb_id);
	munmap(buf->vaddr, buf->size);
	drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
}

static void create_fb_(int fd,uint32_t width, uint32_t height, uint32_t color ,struct framebuffer *buf)
{
	struct drm_mode_create_dumb create = {};
 	struct drm_mode_map_dumb map = {};
	uint32_t i;
	uint32_t fb_id;
 
	create.width = width;
	create.height = height;
	create.bpp = 32;
	drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create);	//创建显存,返回一个handle
 
	drmModeAddFB(fd, create.width, create.height, 24, 32, create.pitch,create.handle, &fb_id); 
	
	map.handle = create.handle;
	drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map);	//显存绑定fd，并根据handle返回offset
 
	//通过offset找到对应的显存(framebuffer)并映射到用户空间
	uint32_t *vaddr = (uint32_t *)mmap(0, create.size, PROT_READ | PROT_WRITE,MAP_SHARED, fd, map.offset);	
 
	// for (i = 0; i < (create.size / 4); i++)
	// 	vaddr[i] = color;
    
    buf->height = height;
    buf->width = width;
	buf->vaddr=vaddr;
	buf->handle=create.handle;
	buf->size=create.size;
	buf->fb_id=fb_id;
 
	return;
}

static void create_fb(int fd,uint32_t width, uint32_t height ,struct framebuffer *buf)
{
	struct drm_mode_create_dumb create = {};
 	struct drm_mode_map_dumb map = {};
	uint32_t i;
	uint32_t fb_id;
 
	create.width = width;
	create.height = height;
	create.bpp = 32;
	drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create);	//创建显存,返回一个handle
 
	drmModeAddFB(fd, create.width, create.height, 24, 32, create.pitch,create.handle, &fb_id); 
	
	map.handle = create.handle;
	drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map);	//显存绑定fd，并根据handle返回offset
 
	//通过offset找到对应的显存(framebuffer)并映射到用户空间
	uint32_t *vaddr = (uint32_t *)mmap(0, create.size, PROT_READ | PROT_WRITE,MAP_SHARED, fd, map.offset);	
 
    buf->height = height;
    buf->width = width;
	buf->vaddr=vaddr;
	buf->handle=create.handle;
	buf->size=create.size;
	buf->fb_id=fb_id;
 
	return;
}

void mdelay(int milliseconds)
{
    auto start = std::chrono::high_resolution_clock::now();
    auto end = start + std::chrono::milliseconds(milliseconds);

    while (std::chrono::high_resolution_clock::now() < end)
    {
        // 等待计时器到达指定时间
    }
}

uint32_t g_color = 0, trend_flag = 0;
static uint32_t draw_fb(struct framebuffer *buf)
{
	uint32_t i = 0,h,w;
	
    // 这里绘制图像
    // printf("create size = %d, create.w = %d, create.h = %d\n",create.size,create.width,create.height);

    // 屏幕缓慢变色+鼠标线条
    if (trend_flag == 0){
        g_color++;
        if (g_color >= 0xf0){
            trend_flag = 1;
        }
    }else{
        g_color--;
        if (g_color <= 10){
            trend_flag = 0;
        }
    }
    
    for (h = 0; h < buf->height; h++){
        for (w = 0; w < buf->width; w++){
            uint32_t pix_num = h*buf->width + w;
            if (std::abs(static_cast<int>(h - g_mp.y)) <= MIW && w==g_mp.x){
                // printf("pos %dx%d is 0xff\n",w,h);
                buf->vaddr[pix_num] = 0xffffffff;
            }else{
                buf->vaddr[pix_num] = g_color;
            }
        }
    }



    // 绘制方块
    // for (h = 0; h < buf->height; h++){
    //     for (w = 0; w < buf->width; w++){
    //         uint32_t pix_num = h*buf->width + w;
    //         if (std::abs(static_cast<int>(h - g_mp.y)) <= MIW && std::abs(static_cast<int>(w - g_mp.x)) <= MIW){
    //             // printf("pos %dx%d is 0xff\n",w,h);
    //             buf->vaddr[pix_num] = 0;
    //         }else{
    //             buf->vaddr[pix_num] = 0xffffff;
    //         }
    //     }
    // }

    // 绘制竖条
    // for (h = 0; h < buf->height; h++){
    //     for (w = 0; w < buf->width; w++){
    //         uint32_t pix_num = h*buf->width + w;
    //         if (std::abs(static_cast<int>(h - g_mp.y)) <= MIW && w==g_mp.x){
    //             // printf("pos %dx%d is 0xff\n",w,h);
    //             buf->vaddr[pix_num] = 0;
    //         }else{
    //             buf->vaddr[pix_num] = 0xffffff;
    //         }
    //     }
    // }
        return 0;
    // mdelay(30);
}





int fd;
struct framebuffer buf[BUFFER_TASK_SIZE];
drmModeConnector *connector;
drmModeRes *resources;
uint32_t conn_id;
uint32_t crtc_id;
int connectors_num = 0;
int i = 0;

void *produce_frame_buffer(void *arg, uint32_t *ret){
    struct framebuffer *buf = (struct framebuffer*)arg;
    // printf("buffer info id= %d, pthreadid = %ld\n",buf->fb_id,pthread_self());
    if (buf == NULL){
        return NULL;
    }
    // printf("xiaohu produce frame buffer\n");
    draw_fb(buf);
    return NULL;
}

struct timeval curr_time;
double last_time_stamp, time_stamp;
void *consume_frame_buffer(void *arg, uint32_t *ret){
    double one_frame_time;
    last_time_stamp = time_stamp;
    gettimeofday(&curr_time,NULL);
    time_stamp = (curr_time.tv_sec*1000000.0 +curr_time.tv_usec)/1000;
    one_frame_time = time_stamp - last_time_stamp;

    printf("[%lf] one frame time = %lf, fps = %lf\n",time_stamp, one_frame_time, (1000/one_frame_time));
    struct framebuffer *buf = (struct framebuffer*)arg;
    // printf("buffer info id= %d, pthreadid = %ld\n",buf->fb_id,pthread_self());
    if (buf == NULL){
        return NULL;
    }
    // printf("xiaohu consume frame buffer\n");
    drmModeSetCrtc(fd,crtc_id,buf->fb_id,0,0,&conn_id,1,&connector->modes[0]);
}

int main(int argc, char **argv)
{
    // 显示屏
	fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);	//打开card0，card0一般绑定HDMI和LVDS
 
	resources = drmModeGetResources(fd);	//获取drmModeRes资源,包含fb、crtc、encoder、connector等
    printf("xiaohu get resouece fbs count = %d crtcs count = %d, connectors count = %d, encoders count =%d\n",resources->count_fbs,resources->count_crtcs,resources->count_connectors,resources->count_encoders);
    
    connectors_num = resources->count_connectors;
    for ( i = 0; i < connectors_num; i++ ){
        crtc_id = resources->crtcs[1];  //获取crtc id
        conn_id = resources->connectors[1];  //获取connector id
        connector = drmModeGetConnector(fd, conn_id);	//根据connector_id获取connector资源
        printf("xiaohu, modes count = %d, props count = %d, encoder count = %d\n",connector->count_modes, connector->count_props, connector->count_encoders);
        if (connector == NULL){
            printf("xiaohu ,get connector failed\n");
            return -1;
        }
        if (connector->count_modes == 0) {
            printf("xiahu ,get connector mode count failed\n");
            return -2;
        }
    }
    
    if (connector == NULL){
        printf("xiaohu ,get connector failed\n");
        return -1;
    }
    if (connector->count_modes == 0){
        printf("xiahu ,get connector mode count failed\n");
        return -2;
    }

	printf("hdisplay:%d vdisplay:%d\n",connector->modes[0].hdisplay,connector->modes[0].vdisplay);
 


    // 触摸事件
    init_mp(0,0,800,1280);
    struct callback_func cf;
    cf.input_callback = test_input_callback;
    cf.event_callback = test_event_callback;
    init_input(&cf);

    // 多缓冲模型
    struct buffer_task *buffer_task;
    buffer_task = (struct buffer_task *)malloc(BUFFER_TASK_SIZE * sizeof(struct buffer_task));
    for (i = 0; i < BUFFER_TASK_SIZE; i++){
        create_fb(fd,connector->modes[0].hdisplay,connector->modes[0].vdisplay,&(buf[i]));
        buffer_task[i].args =  (void *)&(buf[i]);
        buffer_task[i].produce_task = produce_frame_buffer;
        buffer_task[i].consume_task = consume_frame_buffer;
    }
    
    M_buffer m_buffer(&buffer_task,BUFFER_TASK_SIZE);
    m_buffer.start_m_buffer_thread();
 

    for(;;){
        usleep(10000000);
    }
    release_fb(fd, &buf[0]);
	release_fb(fd, &buf[1]);
	release_fb(fd, &buf[2]);
	drmModeFreeConnector(connector);
	drmModeFreeResources(resources);
 
	close(fd);
 
	return 0;
}








// int main(int argc, char const *argv[])
// {
//     init_mp(0,0,800,1280);
//     struct callback_func cf;
//     cf.input_callback = test_input_callback;
//     cf.event_callback = test_event_callback;
//     init_input(&cf);
//     return 0;
// }
