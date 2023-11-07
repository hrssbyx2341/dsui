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

#include "xf86drm.h"
#include "xf86drmMode.h"
#include "../../inputs/input_reader.h"

#define DSLOG_TAG "INPUT_SHOW"
#include "../../includes/ds_log.h"

#define MIW 10
struct mouse_pos{
   int32_t x;
   int32_t y;
   int32_t max_x;
   int32_t max_y;
   int32_t min_x;
   int32_t min_y;
};

static struct mouse_pos g_mp;

void init_mp(int32_t min_x, int32_t min_y, int32_t max_x, int32_t max_y){
    g_mp.min_x = min_x;
    g_mp.min_y = min_y;
    g_mp.max_x = max_x;
    g_mp.max_y = max_y;
    g_mp.x = (max_x+min_x)/2;
    g_mp.y = (max_y+min_y)/2;
    //TODO 这里开始显示鼠标点
}

int8_t test_input_callback(struct input_event event, int8_t device_id){
    // DSLOGD("Event id = %d, event type = 0x%x, code = 0x%x, value = 0x%x\n",device_id,event.type,event.code,event.value);
    switch (event.type)
    {
    case EV_SYN:
            // DSLOGD("mouse pos = %dx%d\n",g_mp.x,g_mp.y);
            // show_mouse();
        break;
    case EV_REL:
        switch (event.code)
        {
        case REL_X:
            g_mp.x += event.value;
            if(g_mp.x <= g_mp.min_x) g_mp.x = g_mp.min_x;
            if(g_mp.x >= g_mp.max_x) g_mp.x = g_mp.max_x;
            break;
        case REL_Y:
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
    DSLOGD("%s is %s\n",event_name,(change_type==EVENT_ADD?"added":"removed"));
}

#define uint32_t unsigned int 
 
struct framebuffer{
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


int fd;
struct framebuffer buf[3];
drmModeConnector *connector;
drmModeRes *resources;
uint32_t conn_id;
uint32_t crtc_id;
int connectors_num = 0;
uint32_t g_color = 0xffffff;
struct drm_mode_create_dumb create = {};
struct drm_mode_map_dumb map = {};
uint32_t fb_id;
uint32_t *vaddr;


static void create_fb(int fd,uint32_t width, uint32_t height, uint32_t color ,struct framebuffer *buf)
{
	uint32_t i = 0,h,w;
	
    // 这里绘制图像
    // DSLOGD("create size = %d, create.w = %d, create.h = %d\n",create.size,create.width,create.height);
    
    for (h = 0; h < create.height; h++){
        for (w = 0; w < create.width; w++){
            uint32_t pix_num = h*create.width + w;
            // if (h%5 == 0){
            //     vaddr[h*w] = 0;
            // }else{
            //     vaddr[h*w] = color;
            // }
            if (abs(h - g_mp.y) <= MIW && abs(w - g_mp.x) <= MIW){
                // DSLOGD("pos %dx%d is 0xff\n",w,h);
                vaddr[pix_num] = 0;
            }else{
                vaddr[pix_num] = color;
            }
        }
    }
    // DSLOGD("all pix num = %d\n",i);
	// for (i = 0; i < (create.size / 4); i++){
	// 	// vaddr[i] = color;
    //     if (i < 10000){
    //         vaddr[i] = 0;
    //     }else{
    //         vaddr[i] = color;
    //     }
    // }
 
	buf->vaddr=vaddr;
	buf->handle=create.handle;
	buf->size=create.size;
	buf->fb_id=fb_id;
 
	return;
}

int main(int argc, char **argv)
{
    // 显示屏

    int i = 0;
 
	fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);	//打开card0，card0一般绑定HDMI和LVDS
 
	resources = drmModeGetResources(fd);	//获取drmModeRes资源,包含fb、crtc、encoder、connector等
    printf("xiaohu get resouece fbs count = %d crtcs count = %d, connectors count = %d, encoders count =%d\n",resources->count_fbs,resources->count_crtcs,resources->count_connectors,resources->count_encoders);
    
    connectors_num = resources->count_connectors;
    for ( i = 0; i < connectors_num; i++ ){
        crtc_id = resources->crtcs[0];  //获取crtc id
        conn_id = resources->connectors[0];  //获取connector id
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
    
    init_mp(0,0,connector->modes[0].hdisplay,connector->modes[0].vdisplay); // 初始化鼠标位置

    create.width = connector->modes[0].hdisplay;
	create.height = connector->modes[0].vdisplay;
	create.bpp = 32;
	drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create);	//创建显存,返回一个handle
 
	drmModeAddFB(fd, create.width, create.height, 24, 32, create.pitch,create.handle, &fb_id); 
	
	map.handle = create.handle;
	drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map);	//显存绑定fd，并根据handle返回offset
 
	//通过offset找到对应的显存(framebuffer)并映射到用户空间
	vaddr = mmap(0, create.size, PROT_READ | PROT_WRITE,MAP_SHARED, fd, map.offset);	
 
    // create_fb(fd,connector->modes[0].hdisplay,connector->modes[0].vdisplay, g_color, &buf[0]);
    // drmModeSetCrtc(fd, crtc_id, buf[0].fb_id, 0, 0, &conn_id, 1, &connector->modes[0]);
    // 触摸事件
    struct callback_func cf;
    cf.input_callback = test_input_callback;
    cf.event_callback = test_event_callback;
    init_input(&cf);

    DSLOGD("start ui\n");
    for(;;){
        show_mouse();
        // usleep(1000);
    }
 
	release_fb(fd, &buf[0]);
	release_fb(fd, &buf[1]);
	release_fb(fd, &buf[2]);
 
	drmModeFreeConnector(connector);
	drmModeFreeResources(resources);
 
	close(fd);
 
	return 0;
}




int show_mouse(){
    struct timeval start, draw_end,show_end;
    double draw_time,show_time,all_time;

    gettimeofday(&start,NULL);
    create_fb(fd,connector->modes[0].hdisplay,connector->modes[0].vdisplay, g_color, &buf[0]);
    gettimeofday(&draw_end,NULL);
    drmModeSetCrtc(fd, crtc_id, buf[0].fb_id, 0, 0, &conn_id, 1, &connector->modes[0]);
    gettimeofday(&show_end,NULL);
    draw_time = (draw_end.tv_sec - start.tv_sec)*1000000.0 +(draw_end.tv_usec - start.tv_usec);
    show_time = (show_end.tv_sec - draw_end.tv_sec)*1000000.0 +(show_end.tv_usec - draw_end.tv_usec);
    all_time = draw_time+show_time;
    DSLOGD("frame draw spend %f ms, frame show spend %f ms, one frame spend %f ms, fps = %f\n",(draw_time/1000),(show_time/1000),(all_time/1000),(1000/(all_time/1000)));
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
