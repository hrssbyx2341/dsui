#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <linux/input.h>
#include <pthread.h>

#include "hardware_inputs.h"


#define MAX_EVENTS MAX_INPUT_DEVICE
#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_SIZE (MAX_EVENTS * (EVENT_SIZE + 16))

#define DEVICE_PATH_SIZE 64

static struct hardware_input g_his[MAX_EVENTS];
static int32_t g_epoll_fd;
static struct epoll_event g_events[MAX_EVENTS];

int init_his(){
    int i = 0;
    for (i = 0; i < MAX_EVENTS; i++){
        g_his[i].ev.data.fd = -1;
        g_his[i].device_name = NULL;
        g_his[i].is_avaiable = FALSE;
    }
}

int init_epoll_fd(){
    g_epoll_fd = epoll_create1(0);
    if (g_epoll_fd == -1){
        perror("epoll create");
        return -1;
    }
}

int add_his(char *event_name, int32_t epoll_fd){
    int i = 0;
    for (i = 0; i < MAX_EVENTS; i++){
        if (g_his[i].is_avaiable == FALSE){
            if (g_his[i].device_name == NULL){
                g_his[i].device_name = malloc(DEVICE_PATH_SIZE);
                if (g_his[i].device_name == NULL){
                    return -1;
                }
            }
            memset(g_his[i].device_name,0,DEVICE_PATH_SIZE);
            sprintf(g_his[i].device_name,"/dev/input/%s",event_name);
            g_his[i].ev.data.fd = open(g_his[i].device_name,O_RDONLY);
            if (g_his[i].ev.data.fd == -1){
                return -1;
            }
            g_his[i].ev.events = EPOLLIN;
            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, g_his[i].ev.data.fd, &(g_his[i].ev)) == -1){
                close(g_his[i].ev.data.fd);
                return -1;
            }
            g_his[i].is_avaiable = TRUE;
            return 0;
        }
    }
}

int remove_his(char *event_name, int32_t epoll_fd){
    int i = 0;
    for (i = 0; i < MAX_EVENTS; i++){
        if (g_his[i].is_avaiable == TRUE){
            if (strcmp(event_name, g_his[i].device_name) == 0){
                if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, g_his[i].ev.data.fd, &(g_his[i].ev) == -1)){
                    return -1;
                }
                close(g_his[i].ev.data.fd);
                g_his[i].is_avaiable = FALSE;
                return 0;
            }
        }
    }
}

void *event_input_listener(void *arg){
    int nfds,i;
    struct input_event event;
    struct callback_func *cf = (struct callback_func *)(arg);
    for(;;){
        nfds = epoll_wait(g_epoll_fd, g_events, MAX_EVENTS, -1);
        if (nfds == -1){
            perror("epoll wait");
            sleep(1);
            continue;
        }
        for (i = 0; i < nfds; ++i){
            ssize_t bytes = read(g_events[i].data.fd, &event, sizeof(event));
            if (bytes == sizeof(event)){
                cf->input_callback(event,g_events[i].data.fd);
            }
        }
    }
}

void *event_change_listener(void *arg){
    // 初始化 /dev/input/ 目录下的节点
    struct callback_func *cf = (struct callback_func *)(arg);
    DIR *dir = NULL;
    struct dirent *entry;
    dir = opendir("/dev/input");
    if (dir != NULL){
        while ((entry = readdir(dir)) != NULL){
            struct stat filestat;
            char file_path[DEVICE_PATH_SIZE]={0};
            sprintf(file_path,"/dev/input/%s",entry->d_name);
            int ret = stat(file_path,&filestat);
            if (ret == 0){
                if (S_ISDIR(filestat.st_mode)){
                    continue;
                }
                if (strncmp(entry->d_name, "event", 5) == 0){
                    // TODO 这里用来初始化设备列表
                    add_his(entry->d_name,g_epoll_fd);
                    cf->event_callback(entry->d_name,EVENT_ADD);
                    printf("debug, get input dev = %s\n",entry->d_name);
                }
            }
        }
        printf("\n");
    }
    closedir(dir);

    // 开始监听 /dev/input/ 目录下的输入设备变化
    int fd, wd, length, i;
    char buffer[BUF_SIZE];

    fd = inotify_init();
    if (fd < 0) {
        perror("inotify_init");
        return NULL;
    }

    wd = inotify_add_watch(fd, "/dev/input/", IN_CREATE | IN_DELETE);
    if (wd < 0) {
        perror("inotify_add_watch");
        return NULL;
    }

    while (1) {
        length = read(fd, buffer, BUF_SIZE);
        if (length < 0) {
            perror("read");
            return NULL;
        }
        i = 0;
        while (i < length) {
            struct inotify_event* event = (struct inotify_event*)&buffer[i];
            if (event->len) {
                if (event->mask & IN_CREATE) {
                    if (!(event->mask & IN_ISDIR) && (strncmp(event->name, "event", 5) == 0)) {
                        printf("设备文件 %s 被创建\n", event->name);
                        // TODO 这里要处理设备文件被创建的逻辑
                        add_his(event->name,g_epoll_fd);
                        cf->event_callback(event->name,EVENT_ADD);
                    }
                } else if (event->mask & IN_DELETE) {
                    if (!(event->mask & IN_ISDIR) && (strncmp(event->name, "event", 5) == 0)) {
                        printf("设备文件 %s 被删除\n", event->name);
                        // TODO 这里要处理设备文件被删除的逻辑
                        remove_his(event->name,g_epoll_fd);
                        cf->event_callback(event->name,EVENT_REMOVE);
                    }
                }
            }
            i += EVENT_SIZE + event->len;
        }
    }

    inotify_rm_watch(fd, wd);
    close(fd);

    return NULL;
}

int8_t init_input(struct callback_func *cf){
    pthread_t event_thread_id, input_thread_id;
    init_epoll_fd();
    init_his();
    if (pthread_create(&event_thread_id,NULL,event_change_listener,(void *)cf) != 0){
        perror("event thread");
        return -1;
    }
    
    if (pthread_create(&input_thread_id,NULL,event_input_listener, (void *)cf) != 0){
        perror("input thread");
        return -1;
    }

    for(;;){
        sleep(10);
    }
    return 0;
}

int8_t test_input_callback(struct input_event event, int8_t device_id){
    printf("Event id = %d, event type = 0x%x, code = 0x%x, value = 0x%x\n",device_id,event.type,event.code,event.value);
}

int8_t test_event_callback(char *event_name, int8_t change_type){
    printf("%s is %s\n",event_name,(change_type==EVENT_ADD?"added":"removed"));
}

int main(int argc, char const *argv[])
{
    struct callback_func cf;
    cf.input_callback = test_input_callback;
    cf.event_callback = test_event_callback;
    init_input(&cf);
    return 0;
}
