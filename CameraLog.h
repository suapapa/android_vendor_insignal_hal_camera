#ifndef __ANDROID_HARDWARE_LIBCAMERA_CAMERA_LOG_H__
#define __ANDROID_HARDWARE_LIBCAMERA_CAMERA_LOG_H__

#define LOG_CAMERA_FUNC_ENTER  LOGV("enter %s", __func__)
#define LOG_CAMERA_FUNC_EXIT   LOGV("exit %s", __func__)

static char strFourCC[4 + 1];
static char* getStrFourCC(unsigned int fourCC)
{
    strFourCC[0] = (fourCC >> 24) & 0xff;
    strFourCC[1] = (fourCC >> 16) & 0xff;
    strFourCC[2] = (fourCC >> 8) & 0xff;
    strFourCC[3] = (fourCC) & 0xff;
    strFourCC[4] = 0;

    return strFourCC;
}

//#if defined(LOG_NDEBUG) && LOG_NDEBUG == 0
//#define LOG_
#ifdef CAMERA_LOG_PROFILE

static struct timeval time_start;
static struct timeval time_stop;

unsigned long measure_time(struct timeval* start, struct timeval* stop)
{
    unsigned long sec, usec, time;

    sec = stop->tv_sec - start->tv_sec;

    if (stop->tv_usec >= start->tv_usec) {
        usec = stop->tv_usec - start->tv_usec;
    } else {
        usec = stop->tv_usec + 1000000 - start->tv_usec;
        sec--;
    }

    time = (sec * 1000000) + usec;

    return time;
}

#define LOG_TIME_START(n) \
    struct timeval time_start_##n, time_stop_##n; \
    unsigned long log_time_##n = 0; \
    gettimeofday(&time_start_##n, NULL)

#define LOG_TIME_END(n) \
    gettimeofday(&time_stop_##n, NULL); \
    log_time_##n = measure_time(&time_start_##n, &time_stop_##n)

#define LOG_TIME(n) \
    log_time_##n

#else
#define LOG_CAMERA(...)
#define LOG_CAMERA_PREVIEW(...)
#define LOG_TIME_START(n)
#define LOG_TIME_END(n)
#define LOG_TIME(n)
#endif

#define CHECK(T)                                \
    if (!(T)) {                                 \
        LOGE("%s+%d: Expected %s but failed!!", \
                __func__, __LINE__, #T);        \
    }

#define CHECK_EQ(A, B)                          \
    if (!(A == B)) {                            \
        LOGE("%s+%d: Expected %s(%d) == %s(%d)!!", \
                __func__, __LINE__, #A, A, #B, B);     \
    }

#endif
