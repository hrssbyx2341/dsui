#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#include "hardware_inputs.h"

struct hardware_inputs his[MAX_INPUT_DEVICE];
int8_t g_input_device_index = 0;





void *get_input_device_thread(void *arg){
    DIR *dir = NULL;
    struct dirent *entry;
    // 初始化 hardware_inputs 成员
    // int i = 0;
    // for (i = 0; i < MAX_INPUT_DEVICE; i++){
    //     his[i]->is_avaiable = FALSE;
    // }
    while (1)
    {
        dir = opendir("/dev/input");
        if (dir != NULL){
            while ((entry = readdir(dir)) != NULL){
                char file_path[64]={0};
                struct stat filestat;
                sprintf(file_path,"/dev/input/%s",entry->d_name);
                int ret = stat(file_path,&filestat);
                // printf("debug, ret = %d\n");
                if (ret == 0){
                    if (S_ISDIR(filestat.st_mode)){
                        continue;
                    }
                    printf("debug, get input dev = %s\n",entry->d_name);
                }
            }
            printf("\n");
        }
        closedir(dir);
        usleep(3000000);
    }
    
    return NULL;
}

int8_t main(int argc, char const *argv[])
{
    get_input_device_thread(NULL);
    return 0;
}

// void *input_read_thread(void *arg){
//     DIR *dir;
//     struct dirent *entry;
//     dir = opendir("/dev/input");
//     if (dir == NULL){
        
//     }
    
// }

// int8_t init_input(){

// };