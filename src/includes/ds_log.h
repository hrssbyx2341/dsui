#ifndef __DS_LOG_H__
#define __DS_LOG_H__


#define DSLOG_LEVEL_V 5
#define DSLOG_LEVEL_D 4
#define DSLOG_LEVEL_I 3
#define DSLOG_LEVEL_W 2
#define DSLOG_LEVEL_E 1

#ifndef DSLOG_LEVEL
#define DSLOG_LEVEL DSLOG_LEVEL_V
#endif


#ifndef DSLOG_TAG
#define DSLOG_TAG "DSLOG"
#endif


#ifndef DSLOGV
#define DSLOGV(fmt, ...) \
    if (DSLOG_LEVEL >= 5) printf(("V/"DSLOG_TAG"(%s %d):"fmt),__func__,__LINE__,##__VA_ARGS__)
#endif

#ifndef DSLOGD
#define DSLOGD(fmt, ...) \
    if (DSLOG_LEVEL >= 4) printf(("D/"DSLOG_TAG"(%s %d):"fmt),__func__,__LINE__,##__VA_ARGS__)
#endif

#ifndef DSLOGI
#define DSLOGI(fmt, ...) \
    if (DSLOG_LEVEL >= 3) printf(("I/"DSLOG_TAG"(%s %d):"fmt),__func__,__LINE__,##__VA_ARGS__)
#endif

#ifndef DSLOGW
#define DSLOGW(fmt, ...) \
    if (DSLOG_LEVEL >= 2) printf(("W/"DSLOG_TAG"(%s %d):"fmt),__func__,__LINE__,##__VA_ARGS__)
#endif

#ifndef DSLOGE
#define DSLOGE(fmt, ...) \
    if (DSLOG_LEVEL >= 1) printf(("E/"DSLOG_TAG"(%s %d):"fmt),__func__,__LINE__,##__VA_ARGS__)
#endif


#endif